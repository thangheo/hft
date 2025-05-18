// include/data_ingestion/exchange_manager.hpp
#pragma once

#include "data_ingestion/websocket/exchanges/binance_websocket_client.hpp"
#include "data_ingestion/orderbook/order_book_types.hpp"
#include "data_ingestion/normalization/order_book_normalizer.hpp"
#include "data_ingestion/websocket/connection_pool.hpp"
// #include "data_ingestion/websocket/websocket_client.hpp"
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace trading {
namespace data_ingestion {

// Forward declarations
class ConnectionPool;
class OrderBookNormalizer;
class BinanceWebSocketClient;

// Type definitions
using MarketDataCallback = std::function<void(const MarketDataUpdate&)>;
using NormalizedOrderBookCallback = std::function<void(const OrderBookUpdate&)>;

/**
 * @class ExchangeManager
 * @brief Manages connections to cryptocurrency exchanges and handles market data.
 * 
 * This implementation is optimized for Binance as the primary exchange,
 * with a connection pool for managing multiple WebSocket connections.
 */
class ExchangeManager {
public:
    /**
     * @brief Constructs an ExchangeManager with a callback for normalized order book updates
     * @param callback Function to be called when normalized order book updates are available
     * @param min_connections Minimum number of connections to maintain
     * @param max_connections Maximum number of connections allowed
     */
    ExchangeManager(NormalizedOrderBookCallback callback, 
                   int min_connections = 2,
                   int max_connections = 10);
    
    /**
     * @brief Destructor that ensures proper cleanup of connections
     */
    ~ExchangeManager();
    
    /**
     * @brief Initializes connection with Binance
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Connects to Binance
     * @return true if connection successful, false otherwise
     */
    bool connect();
    
    /**
     * @brief Disconnects from Binance
     */
    void disconnect();
    
    /**
     * @brief Subscribes to market data for specific trading pairs
     * @param trading_pairs List of trading pairs to subscribe to
     * @param data_types Types of data to subscribe to (ORDER_BOOK, TRADES, TICKER)
     * @param depth Depth of order book to request
     * @return true if subscription successful, false otherwise
     */
    bool subscribe(const std::vector<std::string>& trading_pairs,
                  const std::vector<DataType>& data_types,
                  int32_t depth = 10);
    
    /**
     * @brief Checks if connected to Binance
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Gets the current order book for a specific trading pair
     * @param trading_pair The trading pair to get the order book for
     * @param depth Depth of order book to return (0 = full depth)
     * @return The current order book
     */
    OrderBookUpdate getOrderBook(const std::string& trading_pair,
                               int32_t depth = 0) const;
    
    /**
     * @brief Gets connection pool statistics
     * @return String containing connection statistics
     */
    std::string getConnectionStats() const;
#ifdef TESTING
    /**
     * @brief For testing only - allows injection of a mock connection pool
     * @param pool A unique pointer to a mock connection pool
     */
    void setConnectionPoolForTesting(std::unique_ptr<ConnectionPool> pool) {
        connection_pool_ = std::move(pool);
    }
#endif

private:
    // Order book normalizer
    std::unique_ptr<OrderBookNormalizer> normalizer_;
    
    // Connection pool for Binance
    std::unique_ptr<ConnectionPool> connection_pool_;
    
    // Callback for market data updates
    MarketDataCallback data_callback_;
    
    // Thread safety and monitoring
    mutable std::mutex mutex_;
    std::thread monitor_thread_;
    std::atomic<bool> running_;  // Make sure this comes before is_connected_
    bool is_connected_;
    // Connection parameters
    int min_connections_;
    int max_connections_;
    
    // Active subscriptions
    std::vector<SubscriptionRequest> active_subscriptions_;
    
    /**
     * @brief Callback for receiving market data from Binance
     * @param update The market data update
     */
    void onMarketData(const MarketDataUpdate& update);
    
    /**
     * @brief Thread function to monitor connections
     */
    void monitorConnections();
    
    /**
     * @brief Creates Binance-specific stream names for subscription
     * @param trading_pairs List of trading pairs
     * @param data_types Types of data to subscribe to
     * @param depth Depth of order book
     * @return List of formatted stream names
     */
    std::vector<std::string> createStreamNames(
        const std::vector<std::string>& trading_pairs,
        const std::vector<DataType>& data_types,
        int32_t depth);
};

} // namespace data_ingestion
} // namespace trading