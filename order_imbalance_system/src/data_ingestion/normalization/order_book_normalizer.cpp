// src/data_ingestion/order_book_normalizer.cpp
#include "data_ingestion/normalization/order_book_normalizer.hpp"
#include "data_ingestion/protocol/adapter_interface.hpp"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace trading {
namespace data_ingestion {

// OrderBookNormalizer implementation

OrderBookNormalizer::OrderBookNormalizer(NormalizedOrderBookCallback callback)
    : callback_(std::move(callback)) {
    initAdapters();
}

OrderBookNormalizer::~OrderBookNormalizer() = default;

void OrderBookNormalizer::initAdapters() {
    // Add exchange-specific adapters
    exchange_adapters_["binance"] = std::make_shared<BinanceAdapter>();
    // Add other exchanges as needed
}

void OrderBookNormalizer::processMarketData(const MarketDataUpdate& update) {
    // Find adapter for this exchange
    auto it = exchange_adapters_.find(update.exchange_id);
    if (it == exchange_adapters_.end()) {
        std::cerr << "No adapter found for exchange: " << update.exchange_id << std::endl;
        return;
    }
    
    auto adapter = it->second;
    
    // Check if adapter can handle this update
    if (!adapter->canHandle(update)) {
        std::cerr << "Adapter cannot handle update type for exchange: " 
                  << update.exchange_id << std::endl;
        return;
    }
    
    try {
        // Normalize the update
        OrderBookUpdate normalized = adapter->normalize(update);
        
        // Standardize trading pair
        std::string standardized_pair = adapter->standardizeTradingPair(update.trading_pair);
        normalized.trading_pair = standardized_pair;
        
        // Get or create order book
        OrderBook* order_book = getOrCreateOrderBook(update.exchange_id, standardized_pair);
        
        // Forward the normalized update to subscribers
        callback_(normalized);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing market data: " << e.what() << std::endl;
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

void OrderBookNormalizer::processExchangeData(const MarketDataUpdate& update, 
                                            const std::string& exchange_id) {
    // Implementation will be exchange-specific
    // This method would handle parsing/processing for each exchange
}

// BinanceAdapter implementation

bool BinanceAdapter::canHandle(const MarketDataUpdate& update) const {
    if (update.exchange_id != "binance") return false;
    
    return update.update_type == "orderbook" || 
           update.update_type == "orderbook_snapshot";
}

OrderBookUpdate BinanceAdapter::normalize(const MarketDataUpdate& update) const {
    using json = nlohmann::json;
    OrderBookUpdate result;
    
    try {
        // Convert binary data to string
        std::string json_str(update.data.begin(), update.data.end());
        
        // Parse JSON
        json data = json::parse(json_str);
        
        // Fill basic fields
        result.exchange_id = "binance";
        result.trading_pair = update.trading_pair;
        result.exchange_timestamp = update.exchange_timestamp;
        result.received_timestamp = update.received_timestamp;
        
        // Determine if snapshot
        result.is_snapshot = data.contains("lastUpdateId");
        
        // Set sequence number
        if (result.is_snapshot) {
            result.sequence_number = data["lastUpdateId"].get<int64_t>();
        } else {
            result.sequence_number = data["u"].get<int64_t>();
        }
        
        // Parse bids
        if (data.contains("b") || data.contains("bids")) {
            auto& bids_json = data.contains("b") ? data["b"] : data["bids"];
            for (const auto& bid : bids_json) {
                PriceLevel level;
                level.price = bid[0].get<std::string>();
                level.quantity = bid[1].get<std::string>();
                result.bids.push_back(level);
            }
        }
        
        // Parse asks
        if (data.contains("a") || data.contains("asks")) {
            auto& asks_json = data.contains("a") ? data["a"] : data["asks"];
            for (const auto& ask : asks_json) {
                PriceLevel level;
                level.price = ask[0].get<std::string>();
                level.quantity = ask[1].get<std::string>();
                result.asks.push_back(level);
            }
        }
        
        // Set checksum if available
        if (data.contains("checksum")) {
            result.checksum = data["checksum"].get<std::string>();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing Binance data: " << e.what() << std::endl;
        throw;
    }
    
    return result;
}

std::string BinanceAdapter::standardizeTradingPair(const std::string& pair) const {
    // For Binance, convert BTCUSDT to BTC/USDT
    // This is simplified - real implementation would be more robust
    
    // Look for common quote currencies
    const std::vector<std::string> quote_currencies = {
        "USDT", "BTC", "ETH", "BNB", "BUSD", "USDC", "EUR", "USD"
    };
    
    for (const auto& quote : quote_currencies) {
        if (pair.length() > quote.length() && 
            pair.substr(pair.length() - quote.length()) == quote) {
            
            std::string base = pair.substr(0, pair.length() - quote.length());
            return base + "/" + quote;
        }
    }
    
    // If no match, return original with slash in middle (approximate)
    if (pair.length() >= 6) {
        size_t splitPos = pair.length() / 2;
        return pair.substr(0, splitPos) + "/" + pair.substr(splitPos);
    }
    
    // Fallback
    return pair;
}

// AdapterFactory implementation

std::shared_ptr<ExchangeAdapter> AdapterFactory::createAdapter(const std::string& exchange_id) {
    if (exchange_id == "binance") {
        return std::make_shared<BinanceAdapter>();
    }
    
    // Add other exchanges as needed
    
    throw std::runtime_error("No adapter available for exchange: " + exchange_id);
}

} // namespace data_ingestion
} // namespace trading
