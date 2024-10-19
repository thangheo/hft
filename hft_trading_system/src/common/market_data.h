// market_data.h

#ifndef MARKET_DATA_H
#define MARKET_DATA_H


#include <string>
#include <vector>
#include <chrono>

struct OrderBookEntry {
    double price;
    double quantity;
};

struct MarketData {
    std::string exchange;
    std::string symbol;
    std::vector<OrderBookEntry> bids;
    std::vector<OrderBookEntry> asks;
    std::chrono::system_clock::time_point timestamp;
    // Additional fields as needed
};  

#endif // MARKET_DATA_H
