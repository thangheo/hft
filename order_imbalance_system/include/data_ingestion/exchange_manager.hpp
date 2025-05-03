// include/data_ingestion/exchange_manager.hpp
#pragma once

#include "data_ingestion/websocket/exchanges/exchange_connector.hpp"
#include "data_ingestion/websocket/exchanges/binance_websocket_client.hpp"
#include "data_ingestion/orderbook/order_book_types.hpp"
#include "data_ingestion/normalization/order_book_normalizer.hpp"
#include "data_ingestion/websocket/connection_pool.hpp"
#include <map>
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

// Type definitions
using ConnectionPoolFactory = std::function<std::unique_ptr<ConnectionPool>(MarketDataCallback)>;
using MarketDataCallback = std::function<void(const MarketDataUpdate&)>;
using NormalizedOrderBookCallback = std::function<void(const OrderBookUpdate&)>;

/**
 * @class ExchangeManager
 * @brief Manages connections to cryptocurrency exchanges and handles market data.
 * 
 * The ExchangeManager is responsible for:
 * - Managing connections to multiple exchanges
 * - Subscribing to market data streams
 * - Normalizing order book data across exchanges
 * - Monitoring connection health
 */
class ExchangeManager {
public:
    /**
     * @brief Constructs an ExchangeManager with a callback for normalized order book updates
     * @param callback Function to be called when normalized order book updates are available
     */
    ExchangeManager(NormalizedOrderBookCallback callback);
    
    /**
     * @brief Destructor that ensures proper cleanup of connections
     */
    ~ExchangeManager();
    
    /**
     * @brief Registers a new exchange with the manager
     * @param exchange_id Unique identifier for the exchange
     * @param factory Factory function to create connection pools
     * @param min_connections Minimum number of connections to maintain
     * @param max_connections Maximum number of connections allowed
     * @return true if registration successful, false otherwise
     */
    bool registerExchange(const std::string& exchange_id, 
                        ConnectionPoolFactory factory,
                        int min_connections = 1,
                        int max_connections = 10);
    
    /**
     * @brief Removes an exchange from the manager
     * @param exchange_id Identifier of the exchange to remove
     * @return true if removal successful, false otherwise
     */
    bool removeExchange(const std::string& exchange_id);
    
    /**
     * @brief Connects to all registered exchanges
     */
    void connectAll();
    
    /**
     * @brief Disconnects from all exchanges
     */
    void disconnectAll();
    
    /**
     * @brief Subscribes to market data for specific trading pairs on a single exchange
     * @param exchange_id Identifier of the exchange
     * @param trading_pairs List of trading pairs to subscribe to
     * @param data_types Types of data to subscribe to (ORDER_BOOK, TRADES, TICKER)
     * @param depth Depth of order book to request
     * @return true if subscription successful, false otherwise
     */
    bool subscribe(const std::string& exchange_id, 
                  const std::vector<std::string>& trading_pairs,
                  const std::vector<DataType>& data_types,
                  int32_t depth = 10);
    
    /**
     * @brief Subscribes to the same market data across all exchanges
     * @param trading_pairs List of trading pairs to subscribe to
     * @param data_types Types of data to subscribe to
     * @param depth Depth of order book to request
     */
    void subscribeAll(const std::vector<std::string>& trading_pairs,
                     const std::vector<DataType>& data_types,
                     int32_t depth = 10);
    
    /**
     * @brief Gets a list of currently connected exchanges
     * @return Vector of exchange identifiers
     */
    std::vector<std::string> getConnectedExchanges() const;
    
    /**
     * @brief Gets the current order book for a specific exchange and trading pair
     * @param exchange_id Identifier of the exchange
     * @param trading_pair The trading pair to get the order book for
     * @param depth Depth of order book to return (0 = full depth)
     * @return The current order book
     */
    OrderBookUpdate getOrderBook(const std::string& exchange_id, 
                               const std::string& trading_pair,
                               int32_t depth = 0) const;
    
private:
    /** 
     * @brief Structure to store subscription request details
     */
    struct SubscriptionRequest {
        std::string exchange_id;
        std::vector<std::string> trading_pairs;
        std::vector<DataType> data_types;
        int32_t depth;
    };
    
    /**
     * @brief Structure to store exchange connection information
     */
    struct ExchangeInfo {
        std::string exchange_id;
        std::unique_ptr<ConnectionPool> connection_pool;
        std::vector<SubscriptionRequest> active_subscriptions;
        bool is_connected;
    };
    
    // Map of exchange information keyed by exchange ID
    std::map<std::string, ExchangeInfo> exchanges_;
    
    // Order book normalizer
    std::unique_ptr<OrderBookNormalizer> normalizer_;
    
    // Callback for market data updates
    MarketDataCallback data_callback_;
    
    // Thread safety and monitoring
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    mutable std::mutex mutex_;
    
    /**
     * @brief Callback for receiving market data from exchanges
     * @param update The market data update
     */
    void onMarketData(const MarketDataUpdate& update);
    
    /**
     * @brief Thread function to monitor exchange connections
     */
    void monitorConnections();
    
    /**
     * @brief Creates exchange-specific stream names for subscription
     * @param exchange_id Identifier of the exchange
     * @param trading_pairs List of trading pairs
     * @param data_types Types of data to subscribe to
     * @param depth Depth of order book
     * @return List of formatted stream names
     */
    std::vector<std::string> createStreamNames(const std::string& exchange_id,
                                            const std::vector<std::string>& trading_pairs,
                                            const std::vector<DataType>& data_types,
                                            int32_t depth);
};

} // namespace data_ingestion
} // namespace trading