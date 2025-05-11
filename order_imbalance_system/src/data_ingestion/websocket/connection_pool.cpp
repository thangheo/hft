// src/data_ingestion/websocket/connection_pool.cpp
#include "data_ingestion/websocket/connection_pool.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>

namespace trading {
namespace data_ingestion {

ConnectionPool::ConnectionPool(const std::string& pool_id,
                              ConnectionFactory factory,
                              int min_connections,
                              int max_connections)
    : pool_id_(pool_id),
      connection_factory_(std::move(factory)),
      min_connections_(min_connections),
      max_connections_(max_connections) {
    
    // Validate parameters
    if (min_connections < 1) {
        min_connections_ = 1;
    }
    
    if (max_connections < min_connections_) {
        max_connections_ = min_connections_;
    }
    
    std::cout << "Created connection pool '" << pool_id_ << "' with "
              << min_connections_ << "-" << max_connections_ << " connections" << std::endl;
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

void ConnectionPool::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        return;
    }
    
    std::cout << "Initializing connection pool '" << pool_id_ << "'..." << std::endl;
    
    // Pre-create minimum number of connections
    for (int i = 0; i < min_connections_; ++i) {
        auto conn = createConnection();
        if (conn) {
            ConnectionInfo info{conn, false, std::chrono::steady_clock::now(), 0};
            connections_.push_back(std::move(info));
        }
    }
    
    // Start the monitoring thread
    running_ = true;
    monitor_thread_ = std::thread(&ConnectionPool::monitorConnections, this);
    
    std::cout << "Connection pool '" << pool_id_ << "' initialized with "
              << connections_.size() << " connections" << std::endl;
}

bool ConnectionPool::connectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool all_connected = true;
    for (auto& info : connections_) {
        if (info.connection && !info.connection->isConnected()) {
            if (!info.connection->connect()) {
                all_connected = false;
            }
        }
    }
    return all_connected;
}
void ConnectionPool::disconnectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& info : connections_) {
        if (info.connection && info.connection->isConnected()) {
            info.connection->disconnect();
        }
    }
}
void ConnectionPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!running_) {
            return;
        }
        
        std::cout << "Shutting down connection pool '" << pool_id_ << "'..." << std::endl;
        running_ = false;
    }
    
    // Wake up any threads waiting for connections
    conn_available_.notify_all();
    
    // Wait for the monitor thread to finish
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    // Close all connections
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& info : connections_) {
            if (info.connection) {
                info.connection->disconnect();
                info.connection.reset();
            }
        }
        
        connections_.clear();
    }
    
    std::cout << "Connection pool '" << pool_id_ << "' shut down" << std::endl;
}

std::shared_ptr<WebSocketClientBase> ConnectionPool::acquireConnection(int timeout_ms) {
    connection_requests_++;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Try to get an existing connection from the pool
    {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // First check for any available connections
        for (auto& info : connections_) {
            if (!info.in_use && info.connection && info.connection->isConnected()) {
                info.in_use = true;
                info.last_used = std::chrono::steady_clock::now();
                
                std::cout << "Acquired existing connection from pool '" << pool_id_ << "' ("
                          << getActiveConnectionCount() << " active, "
                          << getAvailableConnectionCount() << " available)" << std::endl;
                
                return info.connection;
            }
        }
        
        // No available connections, can we create a new one?
        if (static_cast<int>(connections_.size()) < max_connections_) {
            auto conn = createConnection();
            if (conn) {
                ConnectionInfo info{conn, true, std::chrono::steady_clock::now(), 0};
                connections_.push_back(std::move(info));
                
                std::cout << "Created new connection for pool '" << pool_id_ << "' ("
                          << getActiveConnectionCount() << " active, "
                          << getAvailableConnectionCount() << " available)" << std::endl;
                
                return connections_.back().connection;
            }
        }
        
        // No connections available and can't create more, wait for one to be released
        std::cout << "Waiting for connection from pool '" << pool_id_ << "' ("
                  << getActiveConnectionCount() << " active, "
                  << getAvailableConnectionCount() << " available)" << std::endl;
        
        auto available = conn_available_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
            [this]() {
                return std::any_of(connections_.begin(), connections_.end(),
                    [](const ConnectionInfo& info) {
                        return !info.in_use && info.connection && info.connection->isConnected();
                    });
            });
        
        if (available) {
            // Find the first available connection
            for (auto& info : connections_) {
                if (!info.in_use && info.connection && info.connection->isConnected()) {
                    info.in_use = true;
                    info.last_used = std::chrono::steady_clock::now();
                    
                    std::cout << "Acquired released connection from pool '" << pool_id_ << "' ("
                              << getActiveConnectionCount() << " active, "
                              << getAvailableConnectionCount() << " available)" << std::endl;
                    
                    return info.connection;
                }
            }
        }
    }
    
    // If we get here, we timed out waiting for a connection
    connection_timeouts_++;
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();
    
    std::cerr << "Timed out waiting for connection from pool '" << pool_id_
              << "' after " << elapsed << "ms" << std::endl;
    
    return nullptr;
}

void ConnectionPool::releaseConnection(std::shared_ptr<WebSocketClientBase> connection) {
    if (!connection) {
        return;
    }
    
    connection_releases_++;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find the connection in our pool
    auto it = std::find_if(connections_.begin(), connections_.end(),
        [&connection](const ConnectionInfo& info) {
            return info.connection.get() == connection.get();
        });
    
    if (it != connections_.end()) {
        it->in_use = false;
        it->last_used = std::chrono::steady_clock::now();
        
        // If the connection is broken, increment its failure count
        if (!connection->isConnected()) {
            it->failure_count++;
            
            std::cout << "Released broken connection to pool '" << pool_id_
                     << "' (failure count: " << it->failure_count << ")" << std::endl;
        } else {
            std::cout << "Released connection to pool '" << pool_id_ << "' ("
                      << getActiveConnectionCount() << " active, "
                      << getAvailableConnectionCount() << " available)" << std::endl;
        }
        
        // Notify one waiting thread that a connection is available
        conn_available_.notify_one();
    } else {
        std::cerr << "WARNING: Released connection not found in pool '" << pool_id_ << "'" << std::endl;
    }
}

bool ConnectionPool::isHealthy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // The pool is healthy if at least one connection is working
    return std::any_of(connections_.begin(), connections_.end(),
        [](const ConnectionInfo& info) {
            return info.connection && info.connection->isConnected();
        });
}

int ConnectionPool::getActiveConnectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return std::count_if(connections_.begin(), connections_.end(),
        [](const ConnectionInfo& info) {
            return info.in_use;
        });
}

int ConnectionPool::getAvailableConnectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return std::count_if(connections_.begin(), connections_.end(),
        [](const ConnectionInfo& info) {
            return !info.in_use && info.connection && info.connection->isConnected();
        });
}

int ConnectionPool::getTotalConnectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

void ConnectionPool::monitorConnections() {
    std::cout << "Starting connection pool monitor for '" << pool_id_ << "'" << std::endl;
    
    while (running_) {
        // Sleep for the prune interval
        std::this_thread::sleep_for(prune_interval_);
        
        if (!running_) {
            break;
        }
        
        // Perform maintenance tasks
        pruneConnections();
        ensureMinConnections();
        removeUnhealthyConnections();
    }
    
    std::cout << "Stopped connection pool monitor for '" << pool_id_ << "'" << std::endl;
}

void ConnectionPool::pruneConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (connections_.size() <= static_cast<size_t>(min_connections_)) {
        return;  // Don't prune if we're at or below the minimum
    }
    
    auto now = std::chrono::steady_clock::now();
    int pruned = 0;
    
    // Identify connections to remove (idle and exceeding the minimum)
    auto it = connections_.begin();
    while (it != connections_.end() && static_cast<int>(connections_.size() - pruned) > min_connections_) {
        if (!it->in_use) {
            auto idle_time = std::chrono::duration_cast<std::chrono::seconds>(now - it->last_used);
            
            if (idle_time > idle_timeout_) {
                std::cout << "Pruning idle connection from pool '" << pool_id_ 
                          << "' (idle for " << idle_time.count() << "s)" << std::endl;
                
                if (it->connection) {
                    it->connection->disconnect();
                    it->connection.reset();
                }
                
                it = connections_.erase(it);
                pruned++;
                continue;
            }
        }
        ++it;
    }
    
    if (pruned > 0) {
        std::cout << "Pruned " << pruned << " idle connections from pool '" << pool_id_ << "'" << std::endl;
    }
}

void ConnectionPool::ensureMinConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Count current healthy connections
    int healthy_count = std::count_if(connections_.begin(), connections_.end(),
        [](const ConnectionInfo& info) {
            return info.connection && info.connection->isConnected();
        });
    
    // Create new connections if we're below the minimum
    if (healthy_count < min_connections_) {
        int to_create = min_connections_ - healthy_count;
        std::cout << "Creating " << to_create << " connections to maintain minimum in pool '" 
                  << pool_id_ << "'" << std::endl;
        
        for (int i = 0; i < to_create; ++i) {
            auto conn = createConnection();
            if (conn) {
                ConnectionInfo info{conn, false, std::chrono::steady_clock::now(), 0};
                connections_.push_back(std::move(info));
            } else {
                connection_failures_++;
            }
        }
    }
}

void ConnectionPool::removeUnhealthyConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove connections with too many failures
    auto it = connections_.begin();
    while (it != connections_.end()) {
        if (it->failure_count >= max_failures_) {
            std::cout << "Removing unhealthy connection from pool '" << pool_id_ 
                      << "' (failures: " << it->failure_count << ")" << std::endl;
            
            if (it->connection) {
                it->connection->disconnect();
                it->connection.reset();
            }
            
            it = connections_.erase(it);
        } else {
            ++it;
        }
    }
}

std::shared_ptr<WebSocketClientBase> ConnectionPool::createConnection() {
    try {
        auto conn = connection_factory_();
        
        if (conn && conn->connect()) {
            return conn;
        } else {
            connection_failures_++;
            std::cerr << "Failed to create/connect new connection for pool '" << pool_id_ << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        connection_failures_++;
        std::cerr << "Exception creating connection for pool '" << pool_id_ << "': " 
                  << e.what() << std::endl;
    }
    
    return nullptr;
}

} // namespace data_ingestion
} // namespace trading
// End of file