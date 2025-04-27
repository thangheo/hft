#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "common/types.hpp"

namespace data_ingestion {

class OrderBook {
public:
    OrderBook();
    ~OrderBook();
    
    // Update the order book with new data
    void update(const std::string& instrument, 
                const std::vector<common::PriceLevel>& bids,
                const std::vector<common::PriceLevel>& asks);
    
    // Process any pending updates
    void processUpdates();
    
    // Get current state of the order book
    std::vector<common::PriceLevel> getBids(const std::string& instrument, size_t depth = 10) const;
    std::vector<common::PriceLevel> getAsks(const std::string& instrument, size_t depth = 10) const;
    
    // Calculate mid price
    double getMidPrice(const std::string& instrument) const;
    
    // Get total volume at specified levels
    double getBidVolume(const std::string& instrument, size_t depth = 10) const;
    double getAskVolume(const std::string& instrument, size_t depth = 10) const;

private:
    struct BookData {
        std::vector<common::PriceLevel> bids;
        std::vector<common::PriceLevel> asks;
        std::mutex mutex;
    };
    
    std::unordered_map<std::string, BookData> books_;
    mutable std::mutex mutex_;
};

} // namespace data_ingestion
