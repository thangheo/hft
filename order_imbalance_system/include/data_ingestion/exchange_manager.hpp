// include/data_ingestion/exchange_manager.hpp
#pragma once

#include "data_ingestion/exchange_connector.hpp"
#include "data_ingestion/websocket/connection_pool.hpp"
#include "order_book_normalizer.hpp"
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

/**
 * @brief Manager for exchange connections and data processing
 * 
 * This class manages connections to multiple exchanges, handles subscriptions,
 * and normalizes data across exchanges. It uses connection pools for more
 * reliable and scalable exchange connectivity.
 */
class ExchangeManager {
public:
    /**
     * @brief Factory function type for creating connection pools
     */
    using ConnectionPoolFactory = std::function<std::unique_ptr<ConnectionPool>(MarketDataCallback)>;
    
    /**
     * @brief Constructor
     * 
     * @param callback Callback for normalized order book updates
     */
    ExchangeManager(NormalizedOrderBookCallback callback);
    
    /**
     * @brief Destructor
     */
    ~ExchangeManager();
    
    /**
     * @brief Register an exchange with a connection pool factory
     * 
     * @param exchange_id Exchange identifier
     * @param factory Factory function for creating the exchange's connection pool
     * @param min_connections Minimum number of connections to maintain
     * @param max_connections Maximum number of connections to allow
     * @return true if the exchange was successfully registered
     * @return false if the exchange already exists or registration failed
     */
    bool registerExchange(const std::string& exchange_id, 
                         ConnectionPoolFactory factory,
                         int min_connections = 1,
                         int max_connections = 5);
    
    /**
     * @brief Remove an exchange
     * 
     * @param exchange_id Exchange identifier
     * @return true if the exchange was removed
     * @return false if the exchange doesn't exist
     */
    bool removeExchange(const std::string& exchange_id);
    
    /**
     * @brief Connect to all registered exchanges
     */
    void connectAll();
    
    /**
     * @brief Disconnect from all exchanges
     */
    void disconnectAll();
    
    /**
     * @brief Subscribe to market data for specific trading pairs on an exchange
     * 
     * @param exchange_id Exchange identifier
     * @param trading_pairs List of trading pairs to subscribe to
     * @param data_types Types of data to subscribe to
     * @param depth Order book depth (if applicable)
     * @return true if the subscription was successful
     * @return false if there was an error
     */
    bool subscribe(const std::string& exchange_id, 
                  const std::vector<std::string>& trading_pairs,
                  const std::vector<DataType>& data_types,
                  int32_t depth = 10);
    
    /**
     * @brief Subscribe to the same market data across all exchanges
     * 
     * @param trading_pairs List of trading pairs to subscribe to
     * @param data_types Types of data to subscribe to
     * @param depth Order book depth (if applicable)
     */
    void subscribeAll(const std::vector<std::string>& trading_pairs,
                     const std::vector<DataType>& data_types,
                     int32_t depth = 10);
    
    /**
     * @brief Get a list of connected exchanges
     * 
     * @return std::vector<std::string> List of exchange identifiers
     */
    std::vector<std::string> getConnectedExchanges() const;
    
    /**
     * @brief Get the current order book for a specific exchange and trading pair
     * 
     * @param exchange_id Exchange identifier
     * @param trading_pair Trading pair
     * @param depth Order book depth (0 for full depth)
     * @return OrderBookUpdate The current order book
     */
    OrderBookUpdate getOrderBook(const std::string& exchange_id, 
                                const std::string& trading_pair,
                                int32_t depth = 0) const;
    
private:
    // Exchange connection pools
    struct ExchangeInfo {
        std::string exchange_id;
        std::unique_ptr<ConnectionPool> connection_pool;
        std::vector<SubscriptionRequest> active_subscriptions;
        bool is_connected;
    };
    
    std::map<std::string, ExchangeInfo> exchanges_;
    
    // Market data handling
    std::unique_ptr<OrderBookNormalizer> normalizer_;
    MarketDataCallback data_callback_;
    
    // Connection monitoring
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    mutable std::mutex mutex_;
    
    // Internal methods
    void onMarketData(const MarketDataUpdate& update);
    void monitorConnections();
    
    // Helper for creating subscription stream names (exchange-specific)
    std::vector<std::string> createStreamNames(const std::string& exchange_id,
                                             const std::vector<std::string>& trading_pairs,
                                             const std::vector<DataType>& data_types,
                                             int32_t depth);
};

} // namespace data_ingestion
} // namespace trading