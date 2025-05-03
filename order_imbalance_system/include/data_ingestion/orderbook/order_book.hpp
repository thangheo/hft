/**
 * @file order_book.hpp
 * @brief Definition of order book and related classes
 * 
 * @details 
 * Part of the Data Ingestion & Processing Layer for the Crypto Trading System.
 * 
 * Created on: April 30, 2025
 */
#pragma once

#include "data_ingestion/types.hpp"
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace trading {
namespace data_ingestion {

// Forward declarations
class OrderBook;

// Callback for order book updates
using OrderBookCallback = std::function<void(const OrderBook&)>;

// Order book class
class OrderBook {
public:
    OrderBook(ExchangeId exchange, const Symbol& symbol, OrderBookCallback callback = nullptr);
    ~OrderBook() = default;
    
    // Update methods
    bool applySnapshot(const std::vector<PriceLevel>& bids, 
                      const std::vector<PriceLevel>& asks,
                      uint64_t sequence_number,
                      Timestamp timestamp);
                      
    bool applyUpdate(const std::vector<PriceLevel>& bids,
                    const std::vector<PriceLevel>& asks,
                    uint64_t sequence_number,
                    Timestamp timestamp);
    
    // Query methods
    std::vector<PriceLevel> getBids(int depth = 0) const;
    std::vector<PriceLevel> getAsks(int depth = 0) const;
    
    PriceLevel getBestBid() const;
    PriceLevel getBestAsk() const;
    
    double getMidPrice() const;
    double getSpread() const;
    double getSpreadPercent() const;
    
    // Order book info
    ExchangeId getExchangeId() const { return exchange_id_; }
    Symbol getSymbol() const { return symbol_; }
    uint64_t getSequenceNumber() const { return sequence_number_; }
    Timestamp getTimestamp() const { return timestamp_; }
    bool isValid() const { return is_valid_; }
    
    // Order book metrics
    double getBidVolume(int depth = 0) const;
    double getAskVolume(int depth = 0) const;
    double getVolumeImbalance(int depth = 0) const;
    
    // Calculate checksum for validation
    std::string calculateChecksum() const;
    
private:
    ExchangeId exchange_id_;
    Symbol symbol_;
    OrderBookCallback callback_;
    
    // Order book state
    std::map<double, PriceLevel, std::greater<double>> bids_;  // Descending order for bids
    std::map<double, PriceLevel, std::less<double>> asks_;     // Ascending order for asks
    
    // Metadata
    uint64_t sequence_number_ = 0;
    Timestamp timestamp_;
    std::atomic<bool> is_valid_{false};
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Update price levels
    void updateBidLevel(const PriceLevel& level);
    void updateAskLevel(const PriceLevel& level);
    
    // Notify subscribers about updates
    void notifyCallback();
};
 
} // namespace data_ingestion
} // namespace trading