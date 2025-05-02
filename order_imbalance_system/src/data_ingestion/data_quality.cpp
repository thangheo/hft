// src/data_ingestion/data_quality.cpp
#include "data_ingestion/data_quality.hpp"
#include <iostream>
#include <sstream>

namespace trading {
namespace data_ingestion {

// DataQualityControl implementation

DataQualityControl::DataQualityControl() {
    // Set up standard filter chain
    auto checksum_filter = std::make_shared<ChecksumFilter>();
    auto price_range_filter = std::make_shared<PriceRangeFilter>();
    auto sequence_gap_filter = std::make_shared<SequenceGapFilter>();
    
    // Chain them together
    first_filter_ = checksum_filter;
    checksum_filter->setNext(price_range_filter);
    price_range_filter->setNext(sequence_gap_filter);
    last_filter_ = sequence_gap_filter;
}

void DataQualityControl::addFilter(std::shared_ptr<DataQualityFilter> filter) {
    if (last_filter_) {
        last_filter_->setNext(filter);
        last_filter_ = filter;
    } else {
        first_filter_ = last_filter_ = filter;
    }
}

bool DataQualityControl::validateUpdate(OrderBookUpdate& update) {
    if (!first_filter_) return true;
    return first_filter_->process(update);
}

// ChecksumFilter implementation

bool ChecksumFilter::filter(OrderBookUpdate& update) {
    // Skip if no checksum provided
    if (update.checksum.empty()) return true;
    
    // In a real implementation, compute the checksum and validate
    // For now, always return true
    return true;
}

// PriceRangeFilter implementation

PriceRangeFilter::PriceRangeFilter(const std::unordered_map<std::string, double>& max_prices) 
    : max_prices_(max_prices) {
}

bool PriceRangeFilter::filter(OrderBookUpdate& update) {
    double reasonable_range = getReasonablePriceRange(update.trading_pair);
    
    // Check if any price is outside reasonable range
    for (const auto& level : update.bids) {
        double price = std::stod(level.price);
        if (price <= 0 || price > reasonable_range) {
            std::cerr << "Price range filter failed for bid: " << price 
                      << " (max: " << reasonable_range << ")" << std::endl;
            return false;
        }
    }
    
    for (const auto& level : update.asks) {
        double price = std::stod(level.price);
        if (price <= 0 || price > reasonable_range) {
            std::cerr << "Price range filter failed for ask: " << price 
                      << " (max: " << reasonable_range << ")" << std::endl;
            return false;
        }
    }
    
    return true;
}

double PriceRangeFilter::getReasonablePriceRange(const std::string& symbol) const {
    // First check if we have a specific limit for this symbol
    auto it = max_prices_.find(symbol);
    if (it != max_prices_.end()) {
        return it->second;
    }
    
    // Otherwise use default limits based on typical ranges
    if (symbol.find("BTC") != std::string::npos) return 100000.0;
    if (symbol.find("ETH") != std::string::npos) return 10000.0;
    
    // Default fallback - very high to avoid false positives
    return 1000000.0;
}

// SequenceGapFilter implementation

bool SequenceGapFilter::filter(OrderBookUpdate& update) {
    std::string key = update.exchange_id + ":" + update.trading_pair;
    
    auto it = last_sequence_numbers_.find(key);
    if (it == last_sequence_numbers_.end()) {
        // First update for this symbol
        last_sequence_numbers_[key] = update.sequence_number;
        return true;
    }
    
    // Check for sequence gap
    if (update.sequence_number <= it->second) {
        // Out of order or duplicate
        std::cerr << "Sequence number out of order: " << update.sequence_number 
                  << " <= " << it->second << std::endl;
        return false;
    }
    
    if (update.sequence_number > it->second + 1 && !update.is_snapshot) {
        // Gap detected - may need to request a snapshot
        std::cerr << "Sequence gap detected: " << it->second 
                  << " -> " << update.sequence_number << std::endl;
        // For now, we'll accept it but log the warning
    }
    
    last_sequence_numbers_[key] = update.sequence_number;
    return true;
}

} // namespace data_ingestion
} // namespace trading
