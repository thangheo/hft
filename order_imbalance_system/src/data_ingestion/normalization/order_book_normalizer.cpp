// src/data_ingestion/order_book_normalizer.cpp
#include "data_ingestion/order_book_normalizer.hpp"
#include "data_ingestion/binance_parser.hpp"
#include <iostream>
#include <sstream>

namespace trading {
namespace data_ingestion {

OrderBookNormalizer::OrderBookNormalizer(NormalizedOrderBookCallback callback)
    : callback_(std::move(callback)) {
    initParsers();
}

OrderBookNormalizer::~OrderBookNormalizer() {
    // Clear parsers and order books
    exchange_parsers_.clear();
    order_books_.clear();
}

void OrderBookNormalizer::initParsers() {
    // Initialize exchange-specific parsers
    exchange_parsers_["binance"] = std::make_shared<BinanceParser>();
}

void OrderBookNormalizer::processMarketData(const MarketDataUpdate& update) {
    // Process based on exchange
    if (update.exchange_id == "binance") {
        processBinanceData(update);
    }
    else {
        std::cerr << "Unsupported exchange: " << update.exchange_id << std::endl;
    }
}

OrderBookUpdate OrderBookNormalizer::getOrderBook(const std::string& exchange_id, 
                                                const std::string& trading_pair,
                                                int32_t depth) const {
    // Find the order book
    std::string key = createOrderBookKey(exchange_id, trading_pair);
    auto it = order_books_.find(key);
    
    if (it != order_books_.end()) {
        return it->second->getSnapshot(depth);
    }
    
    // Return an empty order book if not found
    OrderBookUpdate empty;
    empty.exchange_id = exchange_id;
    empty.trading_pair = trading_pair;
    empty.is_snapshot = true;
    return empty;
}

OrderBook* OrderBookNormalizer::getOrCreateOrderBook(const std::string& exchange_id, 
                                                   const std::string& trading_pair) {
    // Create a key for the map
    std::string key = createOrderBookKey(exchange_id, trading_pair);
    
    // Check if the order book exists
    auto it = order_books_.find(key);
    if (it != order_books_.end()) {
        return it->second.get();
    }
    
    // Create a new order book
    std::cout << "Creating new order book for " << exchange_id << ":" << trading_pair << std::endl;
    auto order_book = std::make_unique<OrderBook>(
        exchange_id, trading_pair, 
        [this](const OrderBookUpdate& update) {
            // Forward the update to the callback
            callback_(update);
        }
    );
    
    OrderBook* result = order_book.get();
    order_books_[key] = std::move(order_book);
    return result;
}

std::string OrderBookNormalizer::createOrderBookKey(const std::string& exchange_id, 
                                                  const std::string& trading_pair) const {
    return exchange_id + ":" + trading_pair;
}

void OrderBookNormalizer::processBinanceData(const MarketDataUpdate& update) {
    try {
        // Get the Binance parser
        auto parser_ptr = std::static_pointer_cast<BinanceParser>(exchange_parsers_["binance"]);
        
        // Normalize the trading pair
        std::string normalized_pair = parser_ptr->standardizeTradingPair(update.trading_pair);
        
        // Get or create the order book
        OrderBook* order_book = getOrCreateOrderBook(update.exchange_id, normalized_pair);
        
        // Process the update based on type
        if (update.update_type == "orderbook") {
            // Parse the update into a standard format
            std::vector<uint8_t> data = update.data;
            std::string json_str(data.begin(), data.end());
            
            // Create a MarketDataUpdate with the normalized data
            MarketDataUpdate normalized_update;
            normalized_update.exchange_id = update.exchange_id;
            normalized_update.trading_pair = normalized_pair;
            normalized_update.update_type = update.update_type;
            normalized_update.exchange_timestamp = update.exchange_timestamp;
            normalized_update.received_timestamp = update.received_timestamp;
            normalized_update.data = data;
            
            // Process the update in the order book
            order_book->processUpdate(normalized_update);
        }
        // Other update types can be handled similarly
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing Binance data: " << e.what() << std::endl;
    }
}

} // namespace data_ingestion
} // namespace trading