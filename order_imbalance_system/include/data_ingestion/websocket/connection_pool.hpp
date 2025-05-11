// include/data_ingestion/websocket/connection_pool.hpp
#pragma once

#include "data_ingestion/websocket/client_base.hpp"
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <condition_variable>

namespace trading {
namespace data_ingestion {

// Forward declaration
class WebSocketClientBase;

// Connection factory function type
using ConnectionFactory = std::function<std::shared_ptr<WebSocketClientBase>()>;

class ConnectionPool {
public:
    ConnectionPool(const std::string& pool_id,
                  ConnectionFactory factory,
                  int min_connections = 1,
                  int max_connections = 5);
    ~ConnectionPool();
    
    // Pool lifecycle management
    void initialize();
    void shutdown();
    // Connect all connections in the pool
    bool connectAll();
    
    // Disconnect all connections in the pool
    void disconnectAll();
    // Get a connection from the pool (will block if none available)
    std::shared_ptr<WebSocketClientBase> acquireConnection(int timeout_ms = 5000);
    
    // Release a connection back to the pool
    void releaseConnection(std::shared_ptr<WebSocketClientBase> connection);
    
    // Health monitoring API
    bool isHealthy() const;
    int getActiveConnectionCount() const;
    int getAvailableConnectionCount() const;
    int getTotalConnectionCount() const;
    
private:
    // Connection tracking
    struct ConnectionInfo {
        std::shared_ptr<WebSocketClientBase> connection;
        bool in_use;
        std::chrono::steady_clock::time_point last_used;
        int failure_count;
    };
    
    // Internal methods
    void monitorConnections();
    std::shared_ptr<WebSocketClientBase> createConnection();
    void pruneConnections();
    void ensureMinConnections();
    void removeUnhealthyConnections();
    
    // Pool identification
    std::string pool_id_;
    
    // Connection factory
    ConnectionFactory connection_factory_;
    
    // Pool configuration
    int min_connections_;
    int max_connections_;
    std::chrono::seconds idle_timeout_{300}; // 5 minutes
    int max_failures_{3};
    std::chrono::seconds prune_interval_{60}; // 1 minute
    
    // Connection management
    std::vector<ConnectionInfo> connections_;
    mutable std::mutex mutex_;
    std::condition_variable conn_available_;
    
    // Monitoring thread
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    
    // Statistics
    std::atomic<int> connection_requests_{0};
    std::atomic<int> connection_timeouts_{0};
    std::atomic<int> connection_failures_{0};
    std::atomic<int> connection_releases_{0};
};

} // namespace data_ingestion
} // namespace trading