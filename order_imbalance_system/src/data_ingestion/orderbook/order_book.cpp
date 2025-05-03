/**
 * @file order_book.cpp
 * @brief Implementation of order book and related classes
 * 
 * @details 
 * Part of the Data Ingestion & Processing Layer for the Crypto Trading System.
 * 
 * Created on: April 30, 2025
 */
#include "data_ingestion/orderbook/order_book.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
namespace trading {
namespace data_ingestion {
// namespace orderbook {

OrderBook::OrderBook(ExchangeId exchange, const Symbol& symbol, OrderBookCallback callback)
    : exchange_id_(exchange),
      symbol_(symbol),
      callback_(std::move(callback)) {
}

bool OrderBook::applySnapshot(const std::vector<PriceLevel>& bids, 
                            const std::vector<PriceLevel>& asks,
                            uint64_t sequence_number,
                            Timestamp timestamp) {
    // Skip older snapshots
    if (sequence_number <= sequence_number_) {
        return false;
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
    sequence_number_ = sequence_number;
    timestamp_ = timestamp;
    is_valid_ = true;
    
    // Notify subscribers
    notifyCallback();
    
    return true;
}

bool OrderBook::applyUpdate(const std::vector<PriceLevel>& bids, 
                          const std::vector<PriceLevel>& asks,
                          uint64_t sequence_number,
                          Timestamp timestamp) {
    // Skip out-of-sequence updates
    if (sequence_number <= sequence_number_) {
        return false;
    }
    
    // Detect sequence gaps
    if (is_valid_ && sequence_number > sequence_number_ + 1) {
        // We missed some updates, invalidate the book
        is_valid_ = false;
        std::cerr << "Sequence gap detected for " 
                  << static_cast<int>(exchange_id_) << ":" 
                  << symbol_.base << "/" << symbol_.quote
                  << " (" << sequence_number_ 
                  << " -> " << sequence_number << ")" << std::endl;
        return false;
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
    
    // Update metadata
    sequence_number_ = sequence_number;
    timestamp_ = timestamp;
    
    // Notify subscribers
    notifyCallback();
    
    return true;
}

void OrderBook::updateBidLevel(const PriceLevel& level) {
    double price = level.getPrice();
    
    if (level.getQuantity() <= 0.0) {
        // Remove level if quantity is zero
        bids_.erase(price);
    } else {
        // Update or add level
        bids_[price] = level;
    }
}

void OrderBook::updateAskLevel(const PriceLevel& level) {
    double price = level.getPrice();
    
    if (level.getQuantity() <= 0.0) {
        // Remove level if quantity is zero
        asks_.erase(price);
    } else {
        // Update or add level
        asks_[price] = level;
    }
}

std::vector<PriceLevel> OrderBook::getBids(int depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<PriceLevel> result;
    
    int count = 0;
    for (const auto& [price, level] : bids_) {
        result.push_back(level);
        count++;
        if (depth > 0 && count >= depth) break;
    }
    
    return result;
}

std::vector<PriceLevel> OrderBook::getAsks(int depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<PriceLevel> result;
    
    int count = 0;
    for (const auto& [price, level] : asks_) {
        result.push_back(level);
        count++;
        if (depth > 0 && count >= depth) break;
    }
    
    return result;
}

PriceLevel OrderBook::getBestBid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (bids_.empty()) {
        return PriceLevel();
    }
    
    return bids_.begin()->second;
}

PriceLevel OrderBook::getBestAsk() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (asks_.empty()) {
        return PriceLevel();
    }
    
    return asks_.begin()->second;
}

double OrderBook::getMidPrice() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (bids_.empty() || asks_.empty()) {
        return 0.0;
    }
    
    double best_bid = bids_.begin()->first;
    double best_ask = asks_.begin()->first;
    
    return (best_bid + best_ask) / 2.0;
}

double OrderBook::getSpread() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (bids_.empty() || asks_.empty()) {
        return -1.0;
    }
    
    double best_bid = bids_.begin()->first;
    double best_ask = asks_.begin()->first;
    
    return best_ask - best_bid;
}

double OrderBook::getSpreadPercent() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (bids_.empty() || asks_.empty()) {
        return -1.0;
    }
    
    double best_bid = bids_.begin()->first;
    double best_ask = asks_.begin()->first;
    
    double mid_price = (best_bid + best_ask) / 2.0;
    if (mid_price <= 0.0) {
        return -1.0;
    }
    
    return (best_ask - best_bid) / mid_price * 100.0;
}

double OrderBook::getBidVolume(int depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    double volume = 0.0;
    int count = 0;
    
    for (const auto& [price, level] : bids_) {
        volume += level.getQuantity();
        count++;
        if (depth > 0 && count >= depth) break;
    }
    
    return volume;
}

double OrderBook::getAskVolume(int depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    double volume = 0.0;
    int count = 0;
    
    for (const auto& [price, level] : asks_) {
        volume += level.getQuantity();
        count++;
        if (depth > 0 && count >= depth) break;
    }
    
    return volume;
}

double OrderBook::getVolumeImbalance(int depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    double bid_volume = getBidVolume(depth);
    double ask_volume = getAskVolume(depth);
    
    double total_volume = bid_volume + ask_volume;
    if (total_volume <= 0.0) {
        return 0.0;
    }
    
    return (bid_volume - ask_volume) / total_volume;
}

std::string OrderBook::calculateChecksum() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Simple CRC-like checksum implementation
    uint32_t checksum = 0;
    
    // Include first 10 levels from each side
    int count = 0;
    for (const auto& [price, level] : bids_) {
        if (count >= 10) break;
        
        // Convert price and quantity to integers (scaled by 10^8)
        uint64_t price_int = static_cast<uint64_t>(price * 100000000);
        uint64_t qty_int = static_cast<uint64_t>(level.getQuantity() * 100000000);
        
        // Update checksum with price
        checksum = (checksum * 31) + (price_int & 0xFFFFFFFF);
        checksum = (checksum * 31) + ((price_int >> 32) & 0xFFFFFFFF);
        
        // Update checksum with quantity
        checksum = (checksum * 31) + (qty_int & 0xFFFFFFFF);
        checksum = (checksum * 31) + ((qty_int >> 32) & 0xFFFFFFFF);
        
        count++;
    }
    
    count = 0;
    for (const auto& [price, level] : asks_) {
        if (count >= 10) break;
        
        // Convert price and quantity to integers (scaled by 10^8)
        uint64_t price_int = static_cast<uint64_t>(price * 100000000);
        uint64_t qty_int = static_cast<uint64_t>(level.getQuantity() * 100000000);
        
        // Update checksum with price
        checksum = (checksum * 37) + (price_int & 0xFFFFFFFF);
        checksum = (checksum * 37) + ((price_int >> 32) & 0xFFFFFFFF);
        
        // Update checksum with quantity
        checksum = (checksum * 37) + (qty_int & 0xFFFFFFFF);
        checksum = (checksum * 37) + ((qty_int >> 32) & 0xFFFFFFFF);
        
        count++;
    }
    
    // Format as hexadecimal string
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << checksum;
    
    return ss.str();
}

void OrderBook::notifyCallback() {
    if (!callback_) {
        return;
    }
    
    // Invoke callback with reference to this order book
    callback_(*this);
}

// } // namespace orderbook
} // namespace data_ingestion
} // namespace trading