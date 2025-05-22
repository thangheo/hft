// src/data_ingestion/exchange_manager.cpp
#include "data_ingestion/exchange_manager.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <functional>

namespace trading {
namespace data_ingestion {

ExchangeManager::ExchangeManager(NormalizedOrderBookCallback callback, 
                               int min_connections,
                               int max_connections)
    : normalizedCallback_(std::move(callback)),
      min_connections_(min_connections),
      max_connections_(max_connections),
      is_connected_(false),
      running_(false) {
    
    // Create the normalizer with the callback
    normalizer_ = std::make_unique<OrderBookNormalizer>(normalizedCallback_);
    
    // Create WebSocket callback that converts data and forwards to normalizer
    websocketCallback_ = [this](const std::string& exchange, const std::string& symbol, const std::vector<uint8_t>& data) {
        this->onWebSocketData(exchange, symbol, data);
    };
    
    std::cout << "Exchange Manager initialized" << std::endl;
}

ExchangeManager::~ExchangeManager() {
    // Stop the monitor thread
    running_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    // Disconnect from Binance
    disconnect();
    
    std::cout << "Exchange Manager shutdown complete" << std::endl;
}

bool ExchangeManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Create the connection pool for Binance
        connection_pool_ = std::make_unique<ConnectionPool>(
            "binance",
            // Factory function that properly creates a BinanceWebSocketClient
            [this]() -> ClientInterfacePtr {  // ✅ Correct return type
                return std::make_shared<BinanceWebSocketClient>(
                    "stream.binance.com",  // host
                    "9443",                // port (correct port for WSS)
                    this->websocketCallback_  // ✅ Correct callback type
                );
            },
            min_connections_,
            max_connections_
        );
        
        // Initialize the connection pool
        connection_pool_->initialize();
        
        // Start the connection monitor thread
        running_ = true;
        monitor_thread_ = std::thread(&ExchangeManager::monitorConnections, this);
        
        std::cout << "Binance connection pool initialized with " 
                  << min_connections_ << " to " << max_connections_ << " connections" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing Binance connection pool: " << e.what() << std::endl;
        return false;
    }
}
void ExchangeManager::onWebSocketData(const std::string& exchange, const std::string& symbol, const std::vector<uint8_t>& data) {
    try {
        // Convert WebSocket data to MarketDataUpdate format
        MarketDataUpdate update;
        update.exchangeId = exchange;
        update.tradingPair = symbol;
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        update.data = data;
        
        // Determine update type from the data (you might need to parse JSON to get this)
        // For now, we'll default to "unknown" - you can improve this later
        update.updateType = "unknown";
        
        // Forward to the normalizer
        if (normalizer_) {
            normalizer_->processMarketData(update);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing WebSocket data: " << e.what() << std::endl;
    }
}

bool ExchangeManager::subscribe(const std::vector<std::string>& trading_pairs,
                              const std::vector<DataType>& data_types,
                              int32_t depth) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connection_pool_ || !is_connected_) {
        std::cerr << "Not connected to Binance. Call connect() first." << std::endl;
        return false;
    }
    
    try {
        // Acquire a connection from the pool for subscription
        auto connection = connection_pool_->acquireConnection();
        
        if (!connection) {
            std::cerr << "Failed to acquire connection for subscription" << std::endl;
            return false;
        }
        
        // Convert trading pairs to Symbol objects
        std::vector<core::Symbol> symbols;
        for (const auto& pair : trading_pairs) {
            // Parse trading pair (e.g., "BTC/USDT" or "BTCUSDT")
            core::Symbol symbol = core::Symbol::fromString(pair);
            symbols.push_back(symbol);
        }
        
        // Subscribe using the ClientInterface method
        bool success = connection->subscribe(symbols);
        
        // Release the connection back to the pool
        connection_pool_->releaseConnection(connection);
        
        if (success) {
            // Store the subscription request
            SubscriptionRequest request;
            request.exchangeId = "binance";
            request.trading_pairs = trading_pairs;
            request.data_types = data_types;
            request.depth = depth;
            
            active_subscriptions_.push_back(request);
            
            std::cout << "Subscribed to " << trading_pairs.size() << " trading pairs on Binance" << std::endl;
        } else {
            std::cerr << "Failed to subscribe on Binance" << std::endl;
        }
        
        return success;
    }
    catch (const std::exception& e) {
        std::cerr << "Error subscribing on Binance: " << e.what() << std::endl;
        return false;
    }
}

bool ExchangeManager::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connection_pool_) {
        std::cerr << "Connection pool not initialized. Call initialize() first." << std::endl;
        return false;
    }
    
    try {
        // Pre-connect all connections in the pool
        bool success = connection_pool_->connectAll();
        if (success) {
            is_connected_ = true;
            std::cout << "Connected to Binance" << std::endl;
        } else {
            std::cerr << "Failed to connect to Binance" << std::endl;
        }
        return success;
    }
    catch (const std::exception& e) {
        std::cerr << "Error connecting to Binance: " << e.what() << std::endl;
        return false;
    }
}

void ExchangeManager::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (connection_pool_) {
        try {
            connection_pool_->shutdown();
            is_connected_ = false;
            std::cout << "Disconnected from Binance" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error disconnecting from Binance: " << e.what() << std::endl;
        }
    }
}

// bool ExchangeManager::subscribe(const std::vector<std::string>& trading_pairs,
//                               const std::vector<DataType>& data_types,
//                               int32_t depth) {
//     std::lock_guard<std::mutex> lock(mutex_);
    
//     if (!connection_pool_ || !is_connected_) {
//         std::cerr << "Not connected to Binance. Call connect() first." << std::endl;
//         return false;
//     }
    
//     try {
//         // Create Binance-specific stream names
//         std::vector<std::string> streams = createStreamNames(trading_pairs, data_types, depth);
        
//         if (streams.empty()) {
//             std::cerr << "Failed to create stream names for subscription" << std::endl;
//             return false;
//         }
        
//         // Acquire a connection from the pool for subscription
//         auto connection = connection_pool_->acquireConnection();
        
//         if (!connection) {
//             std::cerr << "Failed to acquire connection for subscription" << std::endl;
//             return false;
//         }
        
//         // Cast to BinanceWebSocketClient and subscribe
//         auto binance_client = std::dynamic_pointer_cast<BinanceWebSocketClient>(connection);
//         if (!binance_client) {
//             std::cerr << "Failed to cast connection to BinanceWebSocketClient" << std::endl;
//             connection_pool_->releaseConnection(connection);
//             return false;
//         }
        
//         // bool success = binance_client->subscribe(streams);
//         bool success = binance_client->subscribeToStreams(streams);
        
//         // Release the connection back to the pool
//         connection_pool_->releaseConnection(connection);
        
//         if (success) {
//             // Store the subscription request
//             SubscriptionRequest request;
//             request.exchangeId = "binance";
//             request.trading_pairs = trading_pairs;
//             request.data_types = data_types;
//             request.depth = depth;
            
//             active_subscriptions_.push_back(request);
            
//             std::cout << "Subscribed to " << trading_pairs.size() << " trading pairs on Binance" << std::endl;
//         } else {
//             std::cerr << "Failed to subscribe on Binance" << std::endl;
//         }
        
//         return success;
//     }
//     catch (const std::exception& e) {
//         std::cerr << "Error subscribing on Binance: " << e.what() << std::endl;
//         return false;
//     }
// }

bool ExchangeManager::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_connected_ && connection_pool_ && connection_pool_->isHealthy();
}

OrderBookUpdate ExchangeManager::getOrderBook(const std::string& tradingPair,
                                            int32_t depth) const {
    return normalizer_->getOrderBook("binance", tradingPair, depth);
}

std::string ExchangeManager::getConnectionStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    
    if (connection_pool_) {
        ss << "Binance connection pool status: " 
           << connection_pool_->getActiveConnectionCount() << " active, "
           << connection_pool_->getAvailableConnectionCount() << " available, "
           << connection_pool_->getTotalConnectionCount() << " total";
    } else {
        ss << "Connection pool not initialized";
    }
    
    return ss.str();
}

// void ExchangeManager::onMarketData(const MarketDataUpdate& update) {
//     // Process the market data through the normalizer
//     normalizer_->processMarketData(update);
// }

void ExchangeManager::monitorConnections() {
    std::cout << "Starting connection monitor thread" << std::endl;
    
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (connection_pool_) {
                // Check if the connection pool is healthy
                bool is_healthy = connection_pool_->isHealthy();
                
                if (is_healthy != is_connected_) {
                    if (is_healthy) {
                        std::cout << "Binance connection restored" << std::endl;
                    } else {
                        std::cerr << "Binance connection lost" << std::endl;
                    }
                    
                    is_connected_ = is_healthy;
                }
                
                // If connection was lost, attempt to restore subscriptions
                if (is_connected_ && !active_subscriptions_.empty()) {
                    for (const auto& subscription : active_subscriptions_) {
                        // Check if we need to resubscribe
                        // if (connection_pool_->needsResubscription()) {
                        if (!connection_pool_->isHealthy()) {
                            std::cerr << "Reconnecting to Binance and restoring subscriptions" << std::endl;
                            // std::cout << "Restoring subscriptions after reconnection" << std::endl;                            
                            // Temporarily unlock to avoid deadlock
                            mutex_.unlock();
                            subscribe(subscription.trading_pairs, subscription.data_types, subscription.depth);
                            mutex_.lock();
                        }
                    }
                }
                
                // Log connection pool statistics periodically
                if (running_ && is_connected_) {
                    std::cout << getConnectionStats() << std::endl;
                }
            }
        }
        
        // Check connections every 30 seconds
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    
    std::cout << "Connection monitor thread stopped" << std::endl;
}

std::vector<std::string> ExchangeManager::createStreamNames(
    const std::vector<std::string>& trading_pairs,
    const std::vector<DataType>& data_types,
    int32_t depth) {
    
    std::vector<std::string> streams;
    
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
    
    return streams;
}

} // namespace data_ingestion
} // namespace trading