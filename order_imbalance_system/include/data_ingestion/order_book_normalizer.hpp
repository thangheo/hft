// include/data_ingestion/order_book_normalizer.hpp
#pragma once

#include "exchange_connector.hpp"
#include "order_book.hpp"
#include <map>
#include <string>
#include <memory>
#include <functional>

namespace trading {
namespace data_ingestion {

// Callback for normalized order book updates
using NormalizedOrderBookCallback = std::function<void(const OrderBookUpdate&)>;

class OrderBookNormalizer {
public:
    OrderBookNormalizer(NormalizedOrderBookCallback callback);
    ~OrderBookNormalizer();
    
    // Process a raw market data update and convert to normalized order book format
    void processMarketData(const MarketDataUpdate& update);
    
    // Get the current normalized order book for a specific exchange and trading pair
    OrderBookUpdate getOrderBook(const std::string& exchange_id, 
                                const std::string& trading_pair,
                                int32_t depth = 0) const;
    
private:
    NormalizedOrderBookCallback callback_;
    
    // Map of exchange-specific parsers
    std::map<std::string, std::shared_ptr<void>> exchange_parsers_;
    
    // Map of order books (exchange_id:trading_pair -> OrderBook)
    std::map<std::string, std::unique_ptr<OrderBook>> order_books_;
    
    // Get or create an order book for the specified exchange and trading pair
    OrderBook* getOrCreateOrderBook(const std::string& exchange_id, 
                                   const std::string& trading_pair);
    
    // Create a key for the order book map
    std::string createOrderBookKey(const std::string& exchange_id, 
                                  const std::string& trading_pair) const;
    
    // Process Binance-specific market data
    void processBinanceData(const MarketDataUpdate& update);
    
    // Initialize exchange-specific parsers
    void initParsers();
};

} // namespace data_ingestion
} // namespace trading