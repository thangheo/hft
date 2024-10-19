#include "backtester.h"
#include <iostream>

Backtester::Backtester() {}

Backtester::~Backtester() {}

bool Backtester::initialize() {
    // Initialize the historical database
    if (!historical_db_.connect("dbname=hft user=postgres password=secret")) {
        std::cerr << "Failed to connect to Historical DB" << std::endl;
        return false;
    }

    // Initialize strategies
    for (auto& strategy : strategies_) {
        if (!strategy->initialize("")) {
            std::cerr << "Failed to initialize strategy" << std::endl;
            return false;
        }
    }

    return true;
}

void Backtester::addStrategy(std::shared_ptr<IStrategy> strategy) {
    strategies_.push_back(strategy);
}

void Backtester::removeStrategy(std::shared_ptr<IStrategy> strategy) {
    strategies_.erase(std::remove(strategies_.begin(), strategies_.end(), strategy), strategies_.end());
}

void Backtester::runBacktest(const std::string& symbol, 
                             const std::chrono::system_clock::time_point& start_time, 
                             const std::chrono::system_clock::time_point& end_time) {
    // Retrieve historical market data
    auto market_data = historical_db_.queryMarketData(symbol, start_time, end_time);

    // Simulate the backtest
    for (const auto& data : market_data) {
        for (auto& strategy : strategies_) {
            strategy->onMarketData(data);
        }

        // Simulate order execution (in reality, you'd need to simulate market impact, slippage, etc.)
        // For simplicity, we'll assume immediate execution at the requested price
    }

    // Perform any final calculations or reporting
}

