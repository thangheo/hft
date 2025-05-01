// src/data_ingestion/exchange_manager.cpp
#include "data_ingestion/exchange_manager.hpp"
#include "data_ingestion/websocket/exchanges/binance_websocket_client.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <functional>

namespace trading {
namespace data_ingestion {

ExchangeManager::ExchangeManager(NormalizedOrderBookCallback callback)
    : normalizer_(std::make_unique<OrderBookNormalizer>(std::move(callback))),
      running_(false) {
    
    // Create market data callback
    data_callback_ = std::bind(&ExchangeManager::onMarketData, this, std::placeholders::_1);
    
    // Start the connection monitor thread
    running_ = true;
    monitor_thread_ = std::thread(&ExchangeManager::monitorConnections, this);
    
    std::cout << "Exchange Manager initialized" << std::endl;
}

ExchangeManager::~ExchangeManager() {
    // Stop the monitor thread
    running_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    // Disconnect from all exchanges
    disconnectAll();
    
    std::cout << "Exchange Manager shutdown complete" << std::endl;
}

bool ExchangeManager::registerExchange(const std::string& exchange_id, 
                                     ConnectionPoolFactory factory,
                                     int min_connections,
                                     int max_connections) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the exchange already exists
    if (exchanges_.find(exchange_id) != exchanges_.end()) {
        std::cerr << "Exchange '" << exchange_id << "' already registered" << std::endl;
        return false;
    }
    
    try {
        // Create the connection pool using the factory
        auto connection_pool = factory(data_callback_);
        
        if (!connection_pool) {
            std::cerr << "Failed to create connection pool for exchange '" << exchange_id << "'" << std::endl;
            return false;
        }
        
        // Create and add the exchange info
        ExchangeInfo info{
            exchange_id,
            std::move(connection_pool),
            {},  // active_subscriptions
            false // is_connected
        };
        
        exchanges_[exchange_id] = std::move(info);
        
        std::cout << "Registered exchange '" << exchange_id << "'" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error registering exchange '" << exchange_id << "': " << e.what() << std::endl;
        return false;
    }
}

bool ExchangeManager::removeExchange(const std::string& exchange_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the exchange exists
    auto it = exchanges_.find(exchange_id);
    if (it == exchanges_.end()) {
        std::cerr << "Exchange '" << exchange_id << "' not found" << std::endl;
        return false;
    }
    
    // Disconnect and shut down the connection pool
    try {
        if (it->second.connection_pool) {
            it->second.connection_pool->shutdown();
        }
        
        // Remove the exchange
        exchanges_.erase(it);
        
        std::cout << "Removed exchange '" << exchange_id << "'" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error removing exchange '" << exchange_id << "': " << e.what() << std::endl;
        return false;
    }
}

void ExchangeManager::connectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : exchanges_) {
        auto& exchange_info = pair.second;
        
        try {
            if (exchange_info.connection_pool) {
                // The pool should already be initialized, so we just mark it as connected
                exchange_info.is_connected = true;
                std::cout << "Connected to exchange '" << exchange_info.exchange_id << "'" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error connecting to exchange '" << exchange_info.exchange_id 
                      << "': " << e.what() << std::endl;
        }
    }
}

void ExchangeManager::disconnectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : exchanges_) {
        auto& exchange_info = pair.second;
        
        try {
            if (exchange_info.connection_pool) {
                exchange_info.connection_pool->shutdown();
                exchange_info.is_connected = false;
                std::cout << "Disconnected from exchange '" << exchange_info.exchange_id << "'" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error disconnecting from exchange '" << exchange_info.exchange_id 
                      << "': " << e.what() << std::endl;
        }
    }
}

bool ExchangeManager::subscribe(const std::string& exchange_id, 
                               const std::vector<std::string>& trading_pairs,
                               const std::vector<DataType>& data_types,
                               int32_t depth) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find the exchange
    auto it = exchanges_.find(exchange_id);
    if (it == exchanges_.end()) {
        std::cerr << "Exchange '" << exchange_id << "' not found" << std::endl;
        return false;
    }
    
    auto& exchange_info = it->second;
    
    try {
        if (!exchange_info.connection_pool) {
            std::cerr << "Connection pool for exchange '" << exchange_id << "' is not initialized" << std::endl;
            return false;
        }
        
        // Create stream names (exchange-specific)
        std::vector<std::string> streams = createStreamNames(
            exchange_id, trading_pairs, data_types, depth);
        
        if (streams.empty()) {
            std::cerr << "Failed to create stream names for exchange '" << exchange_id << "'" << std::endl;
            return false;
        }
        
        // Get a connection from the pool
        auto connection = exchange_info.connection_pool->acquireConnection();
        
        if (!connection) {
            std::cerr << "Failed to acquire connection for exchange '" << exchange_id << "'" << std::endl;
            return false;
        }
        
        bool success = false;
        
        // Subscribe to the streams (exchange-specific)
        if (exchange_id == "binance") {
            auto binance_client = std::dynamic_pointer_cast<BinanceWebSocketClient>(connection);
            if (binance_client) {
                success = binance_client->subscribe(streams);
            }
        }
        // Add other exchanges here as needed
        
        // Release the connection back to the pool
        exchange_info.connection_pool->releaseConnection(connection);
        
        if (success) {
            // Store the subscription request
            SubscriptionRequest request;
            request.exchange_id = exchange_id;
            request.trading_pairs = trading_pairs;
            request.data_types = data_types;
            request.depth = depth;
            
            exchange_info.active_subscriptions.push_back(request);
            
            std::cout << "Subscribed to " << trading_pairs.size() << " trading pairs on exchange '" 
                      << exchange_id << "'" << std::endl;
        } else {
            std::cerr << "Failed to subscribe on exchange '" << exchange_id << "'" << std::endl;
        }
        
        return success;
    }
    catch (const std::exception& e) {
        std::cerr << "Error subscribing on exchange '" << exchange_id << "': " << e.what() << std::endl;
        return false;
    }
}

void ExchangeManager::subscribeAll(const std::vector<std::string>& trading_pairs,
                                  const std::vector<DataType>& data_types,
                                  int32_t depth) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : exchanges_) {
        const std::string& exchange_id = pair.first;
        
        try {
            // Use the existing subscribe method (will acquire its own lock)
            mutex_.unlock();
            bool success = subscribe(exchange_id, trading_pairs, data_types, depth);
            mutex_.lock();
            
            if (!success) {
                std::cerr << "Failed to subscribe to all trading pairs on exchange '" 
                          << exchange_id << "'" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error subscribing to all trading pairs on exchange '" 
                      << exchange_id << "': " << e.what() << std::endl;
        }
    }
}

std::vector<std::string> ExchangeManager::getConnectedExchanges() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    for (const auto& pair : exchanges_) {
        if (pair.second.is_connected) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

OrderBookUpdate ExchangeManager::getOrderBook(const std::string& exchange_id, 
                                            const std::string& trading_pair,
                                            int32_t depth) const {
    return normalizer_->getOrderBook(exchange_id, trading_pair, depth);
}

void ExchangeManager::onMarketData(const MarketDataUpdate& update) {
    // Process the market data through the normalizer
    normalizer_->processMarketData(update);
}

void ExchangeManager::monitorConnections() {
    std::cout << "Starting connection monitor thread" << std::endl;
    
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            for (auto& pair : exchanges_) {
                auto& exchange_info = pair.second;
                
                if (exchange_info.connection_pool) {
                    // Check if the connection pool is healthy
                    bool is_healthy = exchange_info.connection_pool->isHealthy();
                    
                    if (is_healthy != exchange_info.is_connected) {
                        if (is_healthy) {
                            std::cout << "Exchange '" << exchange_info.exchange_id 
                                      << "' connection restored" << std::endl;
                        } else {
                            std::cerr << "Exchange '" << exchange_info.exchange_id 
                                      << "' connection lost" << std::endl;
                        }
                        
                        exchange_info.is_connected = is_healthy;
                    }
                    
                    // Log connection pool statistics periodically
                    if (running_ && is_healthy) {
                        std::cout << "Exchange '" << exchange_info.exchange_id 
                                  << "' pool status: " 
                                  << exchange_info.connection_pool->getActiveConnectionCount() << " active, "
                                  << exchange_info.connection_pool->getAvailableConnectionCount() << " available, "
                                  << exchange_info.connection_pool->getTotalConnectionCount() << " total"
                                  << std::endl;
                    }
                }
            }
        }
        
        // Check connections every 30 seconds
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    
    std::cout << "Connection monitor thread stopped" << std::endl;
}

std::vector<std::string> ExchangeManager::createStreamNames(const std::string& exchange_id,
                                                         const std::vector<std::string>& trading_pairs,
                                                         const std::vector<DataType>& data_types,
                                                         int32_t depth) {
    std::vector<std::string> streams;
    
    // Binance-specific stream name formatting
    if (exchange_id == "binance") {
        for (const auto& pair : trading_pairs) {
            // Convert trading pair to lowercase for Binance
            std::string formatted_pair = pair;
            std::transform(formatted_pair.begin(), formatted_pair.end(), 
                          formatted_pair.begin(), ::tolower);
            
            // Remove separator if present (e.g., "BTC/USDT" -> "btcusdt")
            formatted_pair.erase(std::remove(formatted_pair.begin(), formatted_pair.end(), '/'), 
                               formatted_pair.end());
            
            for (const auto& type : data_types) {
                if (type == DataType::ORDER_BOOK) {
                    // Determine depth update stream based on requested depth
                    if (depth <= 10) {
                        streams.push_back(formatted_pair + "@depth10@100ms");
                    } else if (depth <= 20) {
                        streams.push_back(formatted_pair + "@depth20@100ms");
                    } else {
                        streams.push_back(formatted_pair + "@depth@100ms");
                    }
                    
                    // Also subscribe to book ticker for best bid/ask updates
                    streams.push_back(formatted_pair + "@bookTicker");
                }
                else if (type == DataType::TRADES) {
                    streams.push_back(formatted_pair + "@trade");
                }
                else if (type == DataType::TICKER) {
                    streams.push_back(formatted_pair + "@ticker");
                }
            }
        }
    }
    // Add other exchanges here as needed
    
    return streams;
}

} // namespace data_ingestion
} // namespace trading