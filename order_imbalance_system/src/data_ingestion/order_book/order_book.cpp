// src/data_ingestion/order_book.cpp
#include "data_ingestion/order_book.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <zmq.h>
#include <chrono>
#include <thread>

namespace trading {
namespace data {

OrderBookInstance::OrderBookInstance(const std::string& exchange_id, 
                                   const std::string& trading_pair,
                                   bool track_individual_orders)
    : exchange_id_(exchange_id),
      trading_pair_(trading_pair),
      track_individual_orders_(track_individual_orders),
      last_update_time_(0),
      last_sequence_number_(0),
      is_valid_(false) {}

bool OrderBookInstance::processSnapshot(int64_t sequence_number,
                                      int64_t timestamp,
                                      const std::vector<PriceLevel>& bids,
                                      const std::vector<PriceLevel>& asks) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if this snapshot is newer than our current state
    if (sequence_number <= last_sequence_number_) {
        return false;  // Ignore outdated snapshots
    }
    
    // Clear existing order book
    bids_.clear();
    asks_.clear();
    
    // Add all bid price levels
    for (const auto& bid : bids) {
        updateBidLevel(bid);
    }
    
    // Add all ask price levels
    for (const auto& ask : asks) {
        updateAskLevel(ask);
    }
    
    // Update sequence number and timestamp
    last_sequence_number_ = sequence_number;
    last_update_time_ = timestamp;
    is_valid_ = true;
    
    return true;
}

bool OrderBookInstance::processUpdate(int64_t sequence_number,
                                    int64_t timestamp,
                                    const std::vector<PriceLevel>& bids_to_add,
                                    const std::vector<PriceLevel>& asks_to_add,
                                    const std::vector<std::string>& bids_to_remove,
                                    const std::vector<std::string>& asks_to_remove) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if update is in sequence
    if (sequence_number != last_sequence_number_ + 1) {
        is_valid_ = false;  // Mark as invalid if we missed updates
        return false;
    }
    
    // Process bid updates
    for (const auto& bid : bids_to_add) {
        updateBidLevel(bid);
    }
    
    // Process ask updates
    for (const auto& ask : asks_to_add) {
        updateAskLevel(ask);
    }
    
    // Process bid removals
    for (const auto& price : bids_to_remove) {
        removeBidLevel(price);
    }
    
    // Process ask removals
    for (const auto& price : asks_to_remove) {
        removeAskLevel(price);
    }
    
    // Update sequence number and timestamp
    last_sequence_number_ = sequence_number;
    last_update_time_ = timestamp;
    
    return true;
}

bool OrderBookInstance::getOrderBook(int max_depth,
                                   std::vector<PriceLevel>& bids,
                                   std::vector<PriceLevel>& asks) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_valid_) {
        return false;
    }
    
    bids.clear();
    asks.clear();
    
    // Copy bid levels up to max_depth
    int count = 0;
    for (const auto& bid : bids_) {
        if (count >= max_depth) break;
        bids.push_back(bid.second);
        count++;
    }
    
    // Copy ask levels up to max_depth
    count = 0;
    for (const auto& ask : asks_) {
        if (count >= max_depth) break;
        asks.push_back(ask.second);
        count++;
    }
    
    return true;
}

bool OrderBookInstance::isValid() const {
    return is_valid_;
}

bool OrderBookInstance::getMetrics(std::unordered_map<std::string, double>& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_valid_) {
        return false;
    }
    
    // Calculate total bid and ask volume (first 10 levels)
    double bid_volume = 0.0;
    double ask_volume = 0.0;
    
    int level = 0;
    for (const auto& bid : bids_) {
        if (level >= 10) break;
        bid_volume += std::stod(bid.second.quantity);
        level++;
    }
    
    level = 0;
    for (const auto& ask : asks_) {
        if (level >= 10) break;
        ask_volume += std::stod(ask.second.quantity);
        level++;
    }
    
    // Calculate bid/ask ratio
    double bid_ask_ratio = (ask_volume > 0) ? bid_volume / ask_volume : 0.0;
    
    // Calculate mid price
    double mid_price = 0.0;
    if (!bids_.empty() && !asks_.empty()) {
        double best_bid = std::stod(bids_.begin()->first);
        double best_ask = std::stod(asks_.begin()->first);
        mid_price = (best_bid + best_ask) / 2.0;
    }
    
    // Store metrics
    metrics["bid_volume"] = bid_volume;
    metrics["ask_volume"] = ask_volume;
    metrics["bid_ask_ratio"] = bid_ask_ratio;
    metrics["mid_price"] = mid_price;
    metrics["bid_levels"] = static_cast<double>(bids_.size());
    metrics["ask_levels"] = static_cast<double>(asks_.size());
    
    return true;
}

void OrderBookInstance::updateBidLevel(const PriceLevel& level) {
    if (level.quantity == "0" || level.quantity.empty()) {
        // Remove the level if quantity is zero
        bids_.erase(level.price);
    } else {
        // Update or add the level
        bids_[level.price] = level;
    }
}

void OrderBookInstance::updateAskLevel(const PriceLevel& level) {
    if (level.quantity == "0" || level.quantity.empty()) {
        // Remove the level if quantity is zero
        asks_.erase(level.price);
    } else {
        // Update or add the level
        asks_[level.price] = level;
    }
}

void OrderBookInstance::removeBidLevel(const std::string& price) {
    bids_.erase(price);
}

void OrderBookInstance::removeAskLevel(const std::string& price) {
    asks_.erase(price);
}

// OrderBookReconstructorManager implementation

OrderBookReconstructorManager::OrderBookReconstructorManager()
    : zmq_context_(nullptr),
      zmq_publisher_(nullptr),
      is_publishing_(false) {
    // Initialize ZeroMQ context
    zmq_context_ = zmq_ctx_new();
}

OrderBookReconstructorManager::~OrderBookReconstructorManager() {
    stopPublishing();
    
    // Clean up ZeroMQ resources
    if (zmq_publisher_) {
        zmq_close(zmq_publisher_);
    }
    
    if (zmq_context_) {
        zmq_ctx_destroy(zmq_context_);
    }
}

std::shared_ptr<OrderBookInstance> OrderBookReconstructorManager::getOrCreateOrderBook(
    const std::string& exchange_id,
    const std::string& trading_pair) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = generateKey(exchange_id, trading_pair);
    
    auto it = order_books_.find(key);
    if (it != order_books_.end()) {
        return it->second;
    }
    
    // Create new order book instance
    auto order_book = std::make_shared<OrderBookInstance>(exchange_id, trading_pair);
    order_books_[key] = order_book;
    
    return order_book;
}

bool OrderBookReconstructorManager::removeOrderBook(const std::string& exchange_id,
                                                 const std::string& trading_pair) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = generateKey(exchange_id, trading_pair);
    
    auto it = order_books_.find(key);
    if (it == order_books_.end()) {
        return false;
    }
    
    order_books_.erase(it);
    return true;
}

std::vector<std::shared_ptr<OrderBookInstance>> OrderBookReconstructorManager::getAllOrderBooks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<OrderBookInstance>> result;
    result.reserve(order_books_.size());
    
    for (const auto& pair : order_books_) {
        result.push_back(pair.second);
    }
    
    return result;
}

bool OrderBookReconstructorManager::startPublishing(const std::string& publish_endpoint) {
    if (is_publishing_) {
        return false;  // Already publishing
    }
    
    // Create publisher socket
    zmq_publisher_ = zmq_socket(zmq_context_, ZMQ_PUB);
    if (!zmq_publisher_) {
        return false;
    }
    
    // Bind to endpoint
    if (zmq_bind(zmq_publisher_, publish_endpoint.c_str()) != 0) {
        zmq_close(zmq_publisher_);
        zmq_publisher_ = nullptr;
        return false;
    }
    
    // Start publisher thread
    is_publishing_ = true;
    publisher_thread_ = std::thread(&OrderBookReconstructorManager::publisherThreadFunc, this);
    
    return true;
}

void OrderBookReconstructorManager::stopPublishing() {
    if (!is_publishing_) {
        return;
    }
    
    is_publishing_ = false;
    
    if (publisher_thread_.joinable()) {
        publisher_thread_.join();
    }
}

void OrderBookReconstructorManager::publisherThreadFunc() {
    while (is_publishing_) {
        // Get all order books
        auto order_books = getAllOrderBooks();
        
        for (const auto& order_book : order_books) {
            if (!order_book->isValid()) {
                continue;  // Skip invalid order books
            }
            
            // Get order book state
            std::vector<PriceLevel> bids, asks;
            if (order_book->getOrderBook(10, bids, asks)) {
                // Publish order book update
                // Format message according to Protocol Buffer schema defined in docs
                // TODO: Implement proper Protocol Buffer serialization
                
                std::string topic = "market.orderbook.normalized." + 
                                   order_book->getExchangeId() + "." +
                                   order_book->getTradingPair();
                
                // Simple JSON format for now (to be replaced with Protocol Buffers)
                std::stringstream ss;
                ss << "{";
                ss << "\"exchange_id\":\"" << order_book->getExchangeId() << "\",";
                ss << "\"trading_pair\":\"" << order_book->getTradingPair() << "\",";
                ss << "\"sequence_number\":" << order_book->getSequenceNumber() << ",";
                ss << "\"timestamp\":" << order_book->getLastUpdateTime() << ",";
                
                // Add bids
                ss << "\"bids\":[";
                for (size_t i = 0; i < bids.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << "{\"price\":\"" << bids[i].price << "\",";
                    ss << "\"quantity\":\"" << bids[i].quantity << "\"}";
                }
                ss << "],";
                
                // Add asks
                ss << "\"asks\":[";
                for (size_t i = 0; i < asks.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << "{\"price\":\"" << asks[i].price << "\",";
                    ss << "\"quantity\":\"" << asks[i].quantity << "\"}";
                }
                ss << "]";
                
                ss << "}";
                
                std::string message = ss.str();
                
                // Send message via ZeroMQ
                zmq_send(zmq_publisher_, topic.c_str(), topic.size(), ZMQ_SNDMORE);
                zmq_send(zmq_publisher_, message.c_str(), message.size(), 0);
            }
        }
        
        // Sleep for a short interval to avoid flooding
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

std::string OrderBookReconstructorManager::generateKey(const std::string& exchange_id,
                                                   const std::string& trading_pair) const {
    return exchange_id + ":" + trading_pair;
}

} // namespace data
} // namespace trading