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
#include <boost/crc.hpp> // for crc_32_type
#include <boost/functional/hash.hpp>
namespace trading {
namespace data_ingestion {
// namespace orderbook {

OrderBook::OrderBook(ExchangeId exchange, const Symbol& symbol, OrderBookCallback callback)
    : exchange_id_(exchange),
      symbol_(symbol),
      callback_(std::move(callback)) {
}
OrderBook::OrderBook(const std::string& exchange_id, const std::string& trading_pair, OrderBookCallback callback)
    : exchange_id_(exchange_id),  // Directly assign the string exchange_id
      callback_(std::move(callback)),
      sequence_number_(0),
      is_valid_(false) {
    
    // Parse trading pair
    size_t separator = trading_pair.find('/');
    if (separator != std::string::npos) {
        symbol_.base = trading_pair.substr(0, separator);
        symbol_.quote = trading_pair.substr(separator + 1);
    } else {
        symbol_.base = trading_pair;
        symbol_.quote = "";
    }
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
                  << exchange_id_ << ":" // Now using string directly
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

//getSnapshot
OrderBookUpdate OrderBook::getSnapshot(int depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    OrderBookUpdate update;
    update.exchange_id = exchangeIdToString(exchange_id_);
    update.trading_pair = symbol_.toString();
    update.exchange_timestamp = timestamp_.time_since_epoch().count();
    update.received_timestamp = last_update_time_.time_since_epoch().count();
    update.sequence_number = sequence_number_;
    update.is_snapshot = true;
    
    // Add bid levels
    int bid_count = 0;
    for (const auto& [price, level] : bids_) {
        update.bids.push_back(level);
        bid_count++;
        if (depth > 0 && bid_count >= depth) break;
    }
    
    // Add ask levels
    int ask_count = 0;
    for (const auto& [price, level] : asks_) {
        update.asks.push_back(level);
        ask_count++;
        if (depth > 0 && ask_count >= depth) break;
    }
    
    // Calculate checksum for validation
    update.checksum = calculateChecksum();
    
    return update;
}

void OrderBook::updateBidLevel(const PriceLevel& level) {
    double price = level.getPrice();
    
    if (level.getQuantity() <= 0.0) {
        // Remove the level if quantity is zero or negative
        bids_.erase(price);
    } else {
        // Update or add the level
        bids_[price] = level;
    }
    
    // Update timestamp
    last_update_time_ = std::chrono::system_clock::now();
}

void OrderBook::updateAskLevel(const PriceLevel& level) {
    double price = level.getPrice();
    
    if (level.getQuantity() <= 0.0) {
        // Remove the level if quantity is zero or negative
        asks_.erase(price);
    } else {
        // Update or add the level
        asks_[price] = level;
    }
    
    // Update timestamp
    last_update_time_ = std::chrono::system_clock::now();
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

double OrderBook::getBidVolume(int levels) const {
    double volume = 0.0;
    int count = 0;
    
    for (auto it = bids_.begin(); it != bids_.end() && (levels == 0 || count < levels); ++it, ++count) {
        const auto& level = it->second;
        volume += level.getQuantity();
    }
    
    return volume;
}



double OrderBook::getAskVolume(int levels) const {
    double volume = 0.0;
    int count = 0;
    
    for (auto it = asks_.begin(); it != asks_.end() && (levels == 0 || count < levels); ++it, ++count) {
        const auto& level = it->second;
        volume += level.getQuantity();
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
    std::stringstream ss;
    
    // Add top N levels of bids and asks to the checksum
    const int levels_to_include = 10;
    int bid_count = 0;
    int ask_count = 0;
    
    // Include bids
    for (auto it = bids_.begin(); it != bids_.end() && bid_count < levels_to_include; ++it, ++bid_count) {
        const auto& level = it->second;
        
        // Convert price to 8 decimal places and remove decimal point
        uint64_t price_int = static_cast<uint64_t>(level.getPrice() * 100000000);
        
        // Convert quantity to 8 decimal places and remove decimal point
        uint64_t qty_int = static_cast<uint64_t>(level.getQuantity() * 100000000);
        
        // Add to checksum
        ss << "B:" << price_int << ":" << qty_int << "|";
    }
    
    // Include asks
    for (auto it = asks_.begin(); it != asks_.end() && ask_count < levels_to_include; ++it, ++ask_count) {
        const auto& level = it->second;
        
        // Convert price to 8 decimal places and remove decimal point
        uint64_t price_int = static_cast<uint64_t>(level.getPrice() * 100000000);
        
        // Convert quantity to 8 decimal places and remove decimal point
        uint64_t qty_int = static_cast<uint64_t>(level.getQuantity() * 100000000);
        
        // Add to checksum
        ss << "A:" << price_int << ":" << qty_int << "|";
    }
    
    // Calculate CRC32 of the string
    boost::crc_32_type crc;
    std::string data = ss.str();
    crc.process_bytes(data.data(), data.size());
    
    // Format as hexadecimal
    std::stringstream checksum;
    checksum << std::hex << std::setw(8) << std::setfill('0') << crc.checksum();
    
    return checksum.str();
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