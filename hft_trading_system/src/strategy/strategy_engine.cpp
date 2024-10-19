#include "strategy_engine.h"
#include "src/data_storage/real_time_data/real_time_db.h"
#include "src/common/market_data.h"

StrategyEngine::StrategyEngine() : real_time_db_(std::make_shared<RealTimeDB>()) {}

StrategyEngine::~StrategyEngine() {}

void StrategyEngine::onMarketData(const MarketData& data) {
    // Process market data
    // For example, update the strategy state
}

void StrategyEngine::onOrderUpdate(const Order& order) {
    // Process order update
    // For example, update the strategy state
}

// void StrategyEngine::onExecutionReport(const ExecutionReport& report) {
//     // Process execution report
//     // For example, update the strategy state
// }
void StrategyEngine::setOrderCallback(const std::function<void(const Order&)>& callback) {
    order_callback_ = callback;
}

void StrategyEngine::onTick() {
    MarketData market_data;
    if (real_time_db_->getCachedMarketData("btcusdt", market_data)) {
        // Use the cached market data
        processMarketData(market_data);
    } else {
        // Handle cache miss, possibly wait for the next data update
    }
}
