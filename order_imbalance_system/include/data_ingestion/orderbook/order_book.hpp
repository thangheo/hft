// include/data_ingestion/order_book.hpp
#pragma once

#include "data_ingestion/order_book_types.hpp"
#include <mutex>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace trading {
namespace data_ingestion {

// Single order book for a specific exchange and trading pair
class OrderBook {
public:
    OrderBook(const std::string& exchange_id, 
              const std::string& trading_pair,
              OrderBookCallback callback);
    
    // Process a market data update
    void processUpdate(const MarketDataUpdate& update);
    
    // Get current state of the order book
    OrderBookUpdate getSnapshot(int32_t depth = 0) const;
    
    // Get exchange and pair info
    std::string getExchangeId() const { return exchange_id_; }
    std::string getTradingPair() const { return trading_pair_; }
    int64_t getLastSequenceNumber() const { return last_sequence_number_; }
    
    // Calculate checksum for validation
    std::string calculateChecksum() const;
    
private:
    std::string exchange_id_;
    std::string trading_pair_;
    OrderBookCallback callback_;
    
    // Using maps for sorted price levels
    // Bids stored in descending order (higher prices first)
    std::map<std::string, PriceLevel, std::greater<std::string>> bids_;
    // Asks stored in ascending order (lower prices first)
    std::map<std::string, PriceLevel, std::less<std::string>> asks_;
    
    std::atomic<int64_t> last_sequence_number_{0};
    std::atomic<int64_t> last_timestamp_{0};
    std::atomic<bool> is_valid_{false};
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
    
    // Process different types of updates
    void processSnapshot(const std::vector<PriceLevel>& bids, 
                         const std::vector<PriceLevel>& asks,
                         int64_t sequence_number);
    void processDelta(const std::vector<PriceLevel>& bids, 
                      const std::vector<PriceLevel>& asks,
                      int64_t sequence_number);
    
    // Update price levels
    void updateBidLevel(const PriceLevel& level);
    void updateAskLevel(const PriceLevel& level);
    
    // Notify subscribers about updates
    void notifyUpdate(bool is_snapshot);
};

// Manager for multiple order books
class OrderBookManager {
public:
    OrderBookManager();
    
    // Get or create an order book
    std::shared_ptr<OrderBook> getOrCreateOrderBook(
        const std::string& exchange_id,
        const std::string& trading_pair);
    
    // Get all order books
    std::vector<std::shared_ptr<OrderBook>> getAllOrderBooks() const;
    
    // Process update for a specific order book
    void processUpdate(const MarketDataUpdate& update);
    
private:
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> order_books_;
    mutable std::mutex mutex_;
    
    // Generate a unique key for an order book
    std::string createKey(const std::string& exchange_id, 
                         const std::string& trading_pair) const;
};

} // namespace data_ingestion
} // namespace trading