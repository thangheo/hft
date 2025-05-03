// include/data_ingestion/exchange_manager.hpp
#pragma once

#include "data_ingestion/websocket/exchanges/exchange_connector.hpp"
#include "data_ingestion/orderbook/order_book_types.hpp"
#include "data_ingestion/normalization/order_book_normalizer.hpp"
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace trading {
namespace data_ingestion {

class ExchangeManager {
public:
    ExchangeManager(NormalizedOrderBookCallback callback);
    ~ExchangeManager();
    
    // Add an exchange connector
    bool addExchange(const std::string& exchange_id, std::unique_ptr<ExchangeConnector> connector);
    
    // Remove an exchange connector
    bool removeExchange(const std::string& exchange_id);
    
    // Connect to all exchanges
    void connectAll();
    
    // Disconnect from all exchanges
    void disconnectAll();
    
    // Subscribe to market data for specific trading pairs
    bool subscribe(const std::string& exchange_id, 
                  const std::vector<std::string>& trading_pairs,
                  const std::vector<DataType>& data_types,
                  int32_t depth = 10);
    
    // Subscribe to the same market data across all exchanges
    void subscribeAll(const std::vector<std::string>& trading_pairs,
                     const std::vector<DataType>& data_types,
                     int32_t depth = 10);
    
    // Get a list of connected exchanges
    std::vector<std::string> getConnectedExchanges() const;
    
    // Get the current order book for a specific exchange and trading pair
    OrderBookUpdate getOrderBook(const std::string& exchange_id, 
                                const std::string& trading_pair,
                                int32_t depth = 0) const;
    
private:
    // Map of exchange connectors
    std::map<std::string, std::unique_ptr<ExchangeConnector>> connectors_;
    
    // Order book normalizer
    std::unique_ptr<OrderBookNormalizer> normalizer_;
    
    // Market data callback
    void onMarketData(const MarketDataUpdate& update);
    
    // Connection monitoring
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    mutable std::mutex mutex_;
    
    // Monitor exchange connections
    void monitorConnections();
};

} // namespace data_ingestion
} // namespace trading