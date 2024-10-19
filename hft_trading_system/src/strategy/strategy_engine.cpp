#include "strategy_engine.h"
#include <iostream>
#include <algorithm>  // For std::remove
#include <vector>     // For std::vector

StrategyEngine::StrategyEngine() {}

StrategyEngine::~StrategyEngine() {
    shutdown();
}

bool StrategyEngine::initialize() {
    // Initialize strategies
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& strategy : strategies_) {
        if (!strategy->initialize("")) {
            std::cerr << "Failed to initialize strategy" << std::endl;
            return false;
        }
    }
    return true;
}

// void StrategyEngine::onTick() {
//     MarketData market_data;
//     if (real_time_db_->getCachedMarketData("btcusdt", market_data)) {
//         // Use the cached market data
//         processMarketData(market_data);
//     } else {
//         // Handle cache miss, possibly wait for the next data update
//     }
// }
void StrategyEngine::shutdown() {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& strategy : strategies_) {
        strategy->shutdown();
    }
    strategies_.clear();
}

void StrategyEngine::registerStrategy(std::shared_ptr<IStrategy> strategy) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    strategies_.push_back(strategy);
}

void StrategyEngine::unregisterStrategy(std::shared_ptr<IStrategy> strategy) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    strategies_.erase(std::remove(strategies_.begin(), strategies_.end(), strategy), strategies_.end());
    // strategies_.erase(std::remove(strategies_.begin(), strategies_.end(), strategy), strategies_.end());

}

void StrategyEngine::onMarketData(const MarketData& data) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& strategy : strategies_) {
        strategy->onMarketData(data);
    }
}

void StrategyEngine::onOrderUpdate(const Order& order) {
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    for (auto& strategy : strategies_) {
        strategy->onOrderUpdate(order);
    }
}
