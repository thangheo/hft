#pragma once

#include <vector>
#include <string>
#include "common/types.hpp"
#include "data_ingestion/order_book.hpp"

namespace strategy {

class Analyzer {
public:
    Analyzer(const nlohmann::json& config);
    ~Analyzer();
    
    // Analyze the order book and calculate imbalance metrics
    common::ImbalanceMetrics analyze(const data_ingestion::OrderBook& orderBook);
    
    // Calculate volume imbalance ratio
    double calculateBidAskRatio(const std::vector<common::PriceLevel>& bids,
                               const std::vector<common::PriceLevel>& asks) const;
    
    // Calculate price pressure
    double calculatePricePressure(const std::vector<common::PriceLevel>& bids,
                                 const std::vector<common::PriceLevel>& asks,
                                 double midPrice) const;
    
    // Calculate order flow acceleration
    double calculateAcceleration(const std::string& instrument) const;

private:
    // Configuration parameters
    size_t depth_;
    double weight_decay_;
    
    // Historical metrics for acceleration calculation
    std::unordered_map<std::string, std::vector<double>> historical_ratios_;
};

} // namespace strategy
