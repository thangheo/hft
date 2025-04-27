#include "data_ingestion/order_book.hpp"
#include <algorithm>
#include <iostream>

namespace data_ingestion {

OrderBook::OrderBook() {}

OrderBook::~OrderBook() {}

void OrderBook::update(const std::string& instrument, 
                      const std::vector<common::PriceLevel>& bids,
                      const std::vector<common::PriceLevel>& asks) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& book = books_[instrument];
    
    {
        std::lock_guard<std::mutex> book_lock(book.mutex);
        book.bids = bids;
        book.asks = asks;
    }
}

void OrderBook::processUpdates() {
    // Process any pending updates if needed
    // This method would handle async updates in a real system
}

std::vector<common::PriceLevel> OrderBook::getBids(const std::string& instrument, size_t depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (books_.find(instrument) == books_.end()) {
        return {};
    }
    
    const auto& book = books_.at(instrument);
    std::lock_guard<std::mutex> book_lock(book.mutex);
    
    std::vector<common::PriceLevel> result;
    result.reserve(std::min(depth, book.bids.size()));
    
    for (size_t i = 0; i < std::min(depth, book.bids.size()); ++i) {
        result.push_back(book.bids[i]);
    }
    
    return result;
}

std::vector<common::PriceLevel> OrderBook::getAsks(const std::string& instrument, size_t depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (books_.find(instrument) == books_.end()) {
        return {};
    }
    
    const auto& book = books_.at(instrument);
    std::lock_guard<std::mutex> book_lock(book.mutex);
    
    std::vector<common::PriceLevel> result;
    result.reserve(std::min(depth, book.asks.size()));
    
    for (size_t i = 0; i < std::min(depth, book.asks.size()); ++i) {
        result.push_back(book.asks[i]);
    }
    
    return result;
}

double OrderBook::getMidPrice(const std::string& instrument) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (books_.find(instrument) == books_.end()) {
        return 0.0;
    }
    
    const auto& book = books_.at(instrument);
    std::lock_guard<std::mutex> book_lock(book.mutex);
    
    if (book.bids.empty() || book.asks.empty()) {
        return 0.0;
    }
    
    return (book.bids[0].price + book.asks[0].price) / 2.0;
}

double OrderBook::getBidVolume(const std::string& instrument, size_t depth) const {
    auto bids = getBids(instrument, depth);
    double total = 0.0;
    
    for (const auto& level : bids) {
        total += level.quantity;
    }
    
    return total;
}

double OrderBook::getAskVolume(const std::string& instrument, size_t depth) const {
    auto asks = getAsks(instrument, depth);
    double total = 0.0;
    
    for (const auto& level : asks) {
        total += level.quantity;
    }
    
    return total;
}

} // namespace data_ingestion
