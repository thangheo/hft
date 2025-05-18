// src/data_ingestion/order_book_normalizer.cpp
#include "data_ingestion/normalization/order_book_normalizer.hpp"
#include "data_ingestion/protocol/adapter_interface.hpp"
#include "core/market_types.hpp"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <unordered_map>
//mutex
// #include <mutex>
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
// Add to order_book_normalizer.cpp 

OrderBookUpdate OrderBookNormalizer::getOrderBook(
    const std::string& exchangeId,
    const std::string& trading_pair,
    int depth) const {
    
    std::string key = createOrderBookKey(exchangeId, trading_pair);
    
    // Thread-safe access to order books
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = order_books_.find(key);
    if (it != order_books_.end()) {
        return it->second->getSnapshot(depth);
    }
    
    // Return empty update if not found
    OrderBookUpdate empty_update;
    empty_update.exchangeId = exchangeId;
    empty_update.trading_pair = trading_pair;
    empty_update.is_snapshot = true;
    
    return empty_update;
}
void OrderBookNormalizer::processMarketData(const MarketDataUpdate& update) {
    // Find adapter for this exchange
    auto it = exchange_adapters_.find(update.exchangeId);
    if (it == exchange_adapters_.end()) {
        std::cerr << "No adapter found for exchange: " << update.exchangeId << std::endl;
        return;
    }
    
    auto adapter = it->second;
    
    // Check if adapter can handle this update
    if (!adapter->canHandle(update)) {
        std::cerr << "Adapter cannot handle update type for exchange: " 
                  << update.exchangeId << std::endl;
        return;
    }
    
    try {
        // Normalize the update
        OrderBookUpdate normalized = adapter->normalize(update);
        
        // Standardize trading pair
        std::string standardized_pair = adapter->standardizeTradingPair(update.trading_pair);
        normalized.trading_pair = standardized_pair;
        
        // Get or create order book
        std::shared_ptr<OrderBook> order_book = getOrCreateOrderBook(update.exchangeId, standardized_pair);
        
        // Update the order book based on the normalized data
        if (normalized.is_snapshot) {
            order_book->applySnapshot(
                normalized.bids, 
                normalized.asks, 
                normalized.sequence_number, 
                std::chrono::system_clock::time_point(std::chrono::nanoseconds(normalized.exchange_timestamp))
            );
        } else {
            order_book->applyUpdate(
                normalized.bids, 
                normalized.asks, 
                normalized.sequence_number, 
                std::chrono::system_clock::time_point(std::chrono::nanoseconds(normalized.exchange_timestamp))
            );
        }
        
        // Forward the normalized update to subscribers
        callback_(normalized);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing market data: " << e.what() << std::endl;
    }
}

// std::shared_ptr<OrderBook> OrderBookNormalizer::getOrCreateOrderBook(const std::string& exchangeId, 
//     const std::string& trading_pair) {
//     // Create a key for the map
//     std::string key = createOrderBookKey(exchangeId, trading_pair);

//     // Check if the order book exists
//     auto it = order_books_.find(key);
//     if (it != order_books_.end()) {
//         return it->second;
//     }

//     // Create a new order book
//     std::cout << "Creating new order book for " << exchangeId << ":" << trading_pair << std::endl;

//     // Parse trading pair into base/quote (assuming format like "BTC/USDT")
//     Symbol symbol;
//     size_t slash_pos = trading_pair.find('/');
//     if (slash_pos != std::string::npos) {
//         symbol.base = trading_pair.substr(0, slash_pos);
//         symbol.quote = trading_pair.substr(slash_pos + 1);
//     } else {
//         // Handle case where there's no slash
//         symbol.base = trading_pair;
//         symbol.quote = "";
//     }

//     // Convert exchangeId string to ExchangeId enum
//     trading::core::Exchange exchange_enum = trading::core::Exchange::UNKNOWN;
//     if (exchangeId == "binance") exchange_enum = trading::core::Exchange::BINANCE;
//     else if (exchangeId == "ftx") exchange_enum = trading::core::Exchange::FTX;
//     else if (exchangeId == "coinbase") exchange_enum = trading::core::Exchange::COINBASE;

//     // Create OrderBook with proper callback that forwards updated order book data
//     auto order_book = std::make_shared<OrderBook>(
//         exchange_enum, 
//         symbol,
//         [this](const OrderBook& book) {
//         // Get snapshot from the updated order book and forward to callback
//         OrderBookUpdate update = book.getSnapshot();
//         callback_(update);
//         }
//     );

//     order_books_[key] = order_book;
//     return order_book;
// }
// Modified version of getOrCreateOrderBook method
std::shared_ptr<OrderBook> OrderBookNormalizer::getOrCreateOrderBook(
    const std::string& exchangeId, 
    const std::string& trading_pair) {
    // Create a key for the map
    std::string key = createOrderBookKey(exchangeId, trading_pair);

    // Check if the order book exists
    auto it = order_books_.find(key);
    if (it != order_books_.end()) {
        return it->second;
    }

    // Create a new order book
    std::cout << "Creating new order book for " << exchangeId << ":" << trading_pair << std::endl;

    // Parse trading pair into base/quote (assuming format like "BTC/USDT")
    Symbol symbol;
    size_t slash_pos = trading_pair.find('/');
    if (slash_pos != std::string::npos) {
        symbol.base = trading_pair.substr(0, slash_pos);
        symbol.quote = trading_pair.substr(slash_pos + 1);
    } else {
        // Handle case where there's no slash
        symbol.base = trading_pair;
        symbol.quote = "";
    }

    // Create OrderBook with proper callback that forwards updated order book data
    auto order_book = std::make_shared<OrderBook>(
        exchangeId,  // Now using the string exchangeId directly
        symbol,
        [this](const OrderBook& book) {
            // Get snapshot from the updated order book and forward to callback
            OrderBookUpdate update = book.getSnapshot();
            callback_(update);
        }
    );

    order_books_[key] = order_book;
    return order_book;
}

std::string OrderBookNormalizer::createOrderBookKey(const std::string& exchangeId, 
  const std::string& trading_pair) const {
    return exchangeId + ":" + trading_pair;
}


void OrderBookNormalizer::processExchangeData(const MarketDataUpdate& update, 
                                            const std::string& exchangeId) {
    // Implementation will be exchange-specific
    // This method would handle parsing/processing for each exchange
}

// BinanceAdapter implementation

bool BinanceAdapter::canHandle(const MarketDataUpdate& update) const {
    if (update.exchangeId != "binance") return false;
    
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
        result.exchangeId = "binance";
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
                // Create PriceLevel using the constructor with price and quantity
                double price = std::stod(bid[0].get<std::string>());
                double quantity = std::stod(bid[1].get<std::string>());
                PriceLevel level(price, quantity);                
                // Add to result
                result.bids.push_back(level);
            }
        }
        
        // Parse asks
        if (data.contains("a") || data.contains("asks")) {
            auto& asks_json = data.contains("a") ? data["a"] : data["asks"];
            for (const auto& ask : asks_json) {
                // Create PriceLevel using the constructor with price and quantity
                double price = std::stod(ask[0].get<std::string>());
                double quantity = std::stod(ask[1].get<std::string>());
                PriceLevel level(price, quantity);
                // Add to result
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

std::shared_ptr<ExchangeAdapter> AdapterFactory::createAdapter(const std::string& exchangeId) {
    if (exchangeId == "binance") {
        return std::make_shared<BinanceAdapter>();
    }
    
    // Add other exchanges as needed
    
    throw std::runtime_error("No adapter available for exchange: " + exchangeId);
}

} // namespace data_ingestion
} // namespace trading
