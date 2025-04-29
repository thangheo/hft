// include/data_ingestion/order_book.hpp
#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>
#include <chrono>

namespace trading {
namespace data_ingestion {

struct PriceLevel {
    std::string price;  // As string to preserve precision
    std::string quantity;  // As string to preserve precision
    std::vector<std::pair<std::string, std::string>> orders;  // order_id, quantity pairs
};

struct OrderBookUpdate {
    std::string exchange_id;
    std::string trading_pair;
    int64_t exchange_timestamp;
    int64_t received_timestamp;
    int64_t sequence_number;
    bool is_snapshot;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    std::string checksum;  // For data integrity validation
};

using OrderBookCallback = std::function<void(const OrderBookUpdate&)>;

class OrderBook {
public:
    OrderBook(const std::string& exchange_id, 
              const std::string& trading_pair,
              OrderBookCallback callback);
    
    // Process a market data update to reconstruct the order book
    void processUpdate(const MarketDataUpdate& update);
    
    // Get current state of the order book
    OrderBookUpdate getSnapshot(int32_t depth = 0) const;
    
    // Calculate checksum to verify order book integrity
    std::string calculateChecksum() const;
    
private:
    std::string exchange_id_;
    std::string trading_pair_;
    OrderBookCallback callback_;
    
    mutable std::mutex mutex_;
    std::map<std::string, PriceLevel> bids_;  // price -> PriceLevel
    std::map<std::string, PriceLevel> asks_;  // price -> PriceLevel
    int64_t last_update_id_;
    int64_t last_timestamp_;
    
    void applySnapshot(const MarketDataUpdate& update);
    void applyDelta(const MarketDataUpdate& update);
    void notifyUpdate(bool is_snapshot);
};

} // namespace data_ingestion
} // namespace trading