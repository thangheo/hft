// src/data_ingestion/order_book.cpp
#include "data_ingestion/order_book.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace trading {
namespace data_ingestion {

OrderBook::OrderBook(const std::string& exchange_id, 
                   const std::string& trading_pair,
                   OrderBookCallback callback)
    : exchange_id_(exchange_id),
      trading_pair_(trading_pair),
      callback_(std::move(callback)) {
}

void OrderBook::processUpdate(const MarketDataUpdate& update) {
    // Parse the update based on exchange-specific format
    // In a real implementation, this would use exchange-specific adapters
    // For simplicity, we'll assume the data is already parsed into bids and asks
    
    // Example parsing (placeholder)
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    int64_t sequence_number = 0;
    bool is_snapshot = update.update_type == "orderbook_snapshot";
    
    // In a real implementation, parse update.data according to exchange format
    // For now, we'll just log
    std::cout << "Processing update from " << update.exchange_id 
              << " for " << update.trading_pair 
              << " (type: " << update.update_type << ")" << std::endl;
    
    // Process based on update type
    if (is_snapshot) {
        processSnapshot(bids, asks, sequence_number);
    } else {
        processDelta(bids, asks, sequence_number);
    }
}

void OrderBook::processSnapshot(const std::vector<PriceLevel>& bids, 
                              const std::vector<PriceLevel>& asks,
                              int64_t sequence_number) {
    // Skip older snapshots
    if (sequence_number <= last_sequence_number_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clear existing book
    bids_.clear();
    asks_.clear();
    
    // Add all bid levels
    for (const auto& bid : bids) {
        updateBidLevel(bid);
    }
    
    // Add all ask levels
    for (const auto& ask : asks) {
        updateAskLevel(ask);
    }
    
    // Update metadata
    last_sequence_number_ = sequence_number;
    is_valid_ = true;
    
    // Notify subscribers
    notifyUpdate(true);
}

void OrderBook::processDelta(const std::vector<PriceLevel>& bids, 
                           const std::vector<PriceLevel>& asks,
                           int64_t sequence_number) {
    // Skip out-of-sequence updates
    if (sequence_number <= last_sequence_number_) {
        return;
    }
    
    // Detect sequence gaps
    if (sequence_number > last_sequence_number_ + 1) {
        // We missed some updates, invalidate the book
        is_valid_ = false;
        std::cerr << "Sequence gap detected for " << exchange_id_ << ":" 
                  << trading_pair_ << " (" << last_sequence_number_ 
                  << " -> " << sequence_number << ")" << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Apply bid updates
    for (const auto& bid : bids) {
        updateBidLevel(bid);
    }
    
    // Apply ask updates
    for (const auto& ask : asks) {
        updateAskLevel(ask);
    }
    
    // Update sequence number
    last_sequence_number_ = sequence_number;
    
    // Notify subscribers
    notifyUpdate(false);
}

void OrderBook::updateBidLevel(const PriceLevel& level) {
    // Remove level if quantity is zero
    if (level.quantity == "0" || level.quantity.empty()) {
        bids_.erase(level.price);
    } else {
        // Update or add level
        bids_[level.price] = level;
    }
}

void OrderBook::updateAskLevel(const PriceLevel& level) {
    // Remove level if quantity is zero
    if (level.quantity == "0" || level.quantity.empty()) {
        asks_.erase(level.price);
    } else {
        // Update or add level
        asks_[level.price] = level;
    }
}

OrderBookUpdate OrderBook::getSnapshot(int32_t depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    OrderBookUpdate update;
    update.exchange_id = exchange_id_;
    update.trading_pair = trading_pair_;
    update.sequence_number = last_sequence_number_;
    update.exchange_timestamp = last_timestamp_;
    update.received_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    update.is_snapshot = true;
    
    // Copy bid levels
    int count = 0;
    for (const auto& [price, level] : bids_) {
        if (depth > 0 && count >= depth) break;
        update.bids.push_back(level);
        count++;
    }
    
    // Copy ask levels
    count = 0;
    for (const auto& [price, level] : asks_) {
        if (depth > 0 && count >= depth) break;
        update.asks.push_back(level);
        count++;
    }
    
    // Calculate checksum
    update.checksum = calculateChecksum();
    
    return update;
}

std::string OrderBook::calculateChecksum() const {
    // Simple checksum implementation
    // In production, use CRC32 or similar
    std::stringstream ss;
    
    // Include first 10 levels from each side
    int count = 0;
    for (const auto& [price, level] : bids_) {
        if (count >= 10) break;
        ss << price << ":" << level.quantity << ":";
        count++;
    }
    
    count = 0;
    for (const auto& [price, level] : asks_) {
        if (count >= 10) break;
        ss << price << ":" << level.quantity << ":";
        count++;
    }
    
    std::string data = ss.str();
    if (!data.empty()) {
        data.pop_back(); // Remove trailing colon
    }
    
    // In production, compute actual CRC32
    return "checksum:" + std::to_string(last_sequence_number_);
}

void OrderBook::notifyUpdate(bool is_snapshot) {
    if (!callback_) return;
    
    // Create update for subscribers
    OrderBookUpdate update;
    update.exchange_id = exchange_id_;
    update.trading_pair = trading_pair_;
    update.sequence_number = last_sequence_number_;
    update.is_snapshot = is_snapshot;
    
    // Include top levels (limited for performance)
    int top_levels = 10; // Could make configurable
    
    int count = 0;
    for (const auto& [price, level] : bids_) {
        if (count >= top_levels) break;
        update.bids.push_back(level);
        count++;
    }
    
    count = 0;
    for (const auto& [price, level] : asks_) {
        if (count >= top_levels) break;
        update.asks.push_back(level);
        count++;
    }
    
    // Set timestamps
    update.exchange_timestamp = last_timestamp_;
    update.received_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Set checksum
    update.checksum = calculateChecksum();
    
    // Invoke callback
    callback_(update);
}

// OrderBookManager implementation

OrderBookManager::OrderBookManager() {
}

std::shared_ptr<OrderBook> OrderBookManager::getOrCreateOrderBook(
    const std::string& exchange_id,
    const std::string& trading_pair) {
    
    std::string key = createKey(exchange_id, trading_pair);
    
    // First try without locking for performance
    auto it = order_books_.find(key);
    if (it != order_books_.end()) {
        return it->second;
    }
    
    // If not found, lock and check again
    std::lock_guard<std::mutex> lock(mutex_);
    
    it = order_books_.find(key);
    if (it != order_books_.end()) {
        return it->second;
    }
    
    // Create new order book with default callback
    auto callback = [](const OrderBookUpdate&) {};
    auto book = std::make_shared<OrderBook>(exchange_id, trading_pair, callback);
    order_books_[key] = book;
    
    return book;
}

std::vector<std::shared_ptr<OrderBook>> OrderBookManager::getAllOrderBooks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<OrderBook>> result;
    result.reserve(order_books_.size());
    
    for (const auto& [key, book] : order_books_) {
        result.push_back(book);
    }
    
    return result;
}

void OrderBookManager::processUpdate(const MarketDataUpdate& update) {
    auto book = getOrCreateOrderBook(update.exchange_id, update.trading_pair);
    book->processUpdate(update);
}

std::string OrderBookManager::createKey(const std::string& exchange_id, 
                                      const std::string& trading_pair) const {
    return exchange_id + ":" + trading_pair;
}

} // namespace data_ingestion
} // namespace trading