// src/data_ingestion/data_quality/validator.cpp
#include "data_ingestion/data_quality/validator.hpp"
#include <algorithm>
#include <sstream>

namespace trading {
namespace data_ingestion {

DataValidator::DataValidator() {
    // Add default validation rules
    addRule("bid_ask_order", validateBidAskOrder);
    addRule("non_negative_prices", validateNonNegativePrices);
    addRule("quantities", validateQuantities);
    addRule("bid_ask_spread", validateBidAskSpread);
    addRule("required_fields", validateRequiredFields);
}

void DataValidator::addRule(const std::string& name, ValidationRule rule) {
    rules_[name] = rule;
}

bool DataValidator::validate(const OrderBookUpdate& update, ValidationContext& context) {
    context.reset();
    
    // Apply all validation rules
    for (const auto& rule_pair : rules_) {
        std::string error;
        if (!rule_pair.second(update, error)) {
            context.addError(rule_pair.first + ": " + error);
        }
    }
    
    return context.valid;
}

bool DataValidator::validateBidAskOrder(const OrderBookUpdate& update, std::string& error) {
    // Ensure bids are sorted in descending order
    for (size_t i = 1; i < update.bids.size(); ++i) {
        double prev_price = std::stod(update.bids[i-1].price);
        double curr_price = std::stod(update.bids[i].price);
        
        if (curr_price > prev_price) {
            error = "Bids are not properly sorted (descending order)";
            return false;
        }
    }
    
    // Ensure asks are sorted in ascending order
    for (size_t i = 1; i < update.asks.size(); ++i) {
        double prev_price = std::stod(update.asks[i-1].price);
        double curr_price = std::stod(update.asks[i].price);
        
        if (curr_price < prev_price) {
            error = "Asks are not properly sorted (ascending order)";
            return false;
        }
    }
    
    return true;
}

bool DataValidator::validateNonNegativePrices(const OrderBookUpdate& update, std::string& error) {
    // Check bid prices
    for (const auto& level : update.bids) {
        double price = std::stod(level.price);
        if (price <= 0.0) {
            error = "Found non-positive bid price: " + level.price;
            return false;
        }
    }
    
    // Check ask prices
    for (const auto& level : update.asks) {
        double price = std::stod(level.price);
        if (price <= 0.0) {
            error = "Found non-positive ask price: " + level.price;
            return false;
        }
    }
    
    return true;
}

bool DataValidator::validateQuantities(const OrderBookUpdate& update, std::string& error) {
    // Check bid quantities
    for (const auto& level : update.bids) {
        double quantity = std::stod(level.quantity);
        if (quantity <= 0.0) {
            error = "Found non-positive bid quantity: " + level.quantity;
            return false;
        }
    }
    
    // Check ask quantities
    for (const auto& level : update.asks) {
        double quantity = std::stod(level.quantity);
        if (quantity <= 0.0) {
            error = "Found non-positive ask quantity: " + level.quantity;
            return false;
        }
    }
    
    return true;
}

bool DataValidator::validateBidAskSpread(const OrderBookUpdate& update, std::string& error) {
    if (update.bids.empty() || update.asks.empty()) {
        // Skip validation for empty order books
        return true;
    }
    
    double best_bid = std::stod(update.bids[0].price);
    double best_ask = std::stod(update.asks[0].price);
    
    if (best_bid >= best_ask) {
        error = "Invalid spread: best bid (" + update.bids[0].price + 
                ") >= best ask (" + update.asks[0].price + ")";
        return false;
    }
    
    return true;
}

bool DataValidator::validateRequiredFields(const OrderBookUpdate& update, std::string& error) {
    std::stringstream ss;
    bool valid = true;
    
    if (update.exchange_id.empty()) {
        ss << "Missing exchange_id; ";
        valid = false;
    }
    
    if (update.trading_pair.empty()) {
        ss << "Missing trading_pair; ";
        valid = false;
    }
    
    if (update.exchange_timestamp == 0) {
        ss << "Missing exchange_timestamp; ";
        valid = false;
    }
    
    if (update.received_timestamp == 0) {
        ss << "Missing received_timestamp; ";
        valid = false;
    }
    
    if (update.sequence_number == 0 && !update.is_snapshot) {
        ss << "Missing sequence_number for incremental update; ";
        valid = false;
    }
    
    if (!valid) {
        error = ss.str();
    }
    
    return valid;
}

} // namespace data_ingestion
} // namespace trading