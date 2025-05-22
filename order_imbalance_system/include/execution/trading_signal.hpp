// trading_signal.hpp
#pragma once

#include "execution/models/order.hpp"
#include <string>
#include <chrono>
#include <map>

namespace trading {
namespace execution {

/**
 * Internal representation of a trading signal
 */
struct TradingSignal {
    std::string signal_id;
    std::string tradingPair;
    models::OrderSide direction;  // Using your enum type
    double strength;
    double target_size;
    std::chrono::milliseconds signal_ttl;
    std::string strategy_id;
    std::string strategy_version;
    std::map<std::string, double> signal_metrics;
    
    // Getters for cleaner API access
    const std::string& getSignalId() const { return signal_id; }
    const std::string& getTradingPair() const { return tradingPair; }
    models::OrderSide getDirection() const { return direction; }
    double getStrength() const { return strength; }
    double getTargetSize() const { return target_size; }
    const std::chrono::milliseconds& getSignalTtl() const { return signal_ttl; }
    const std::string& getStrategyId() const { return strategy_id; }
    const std::string& getStrategyVersion() const { return strategy_version; }
    const std::map<std::string, double>& getSignalMetrics() const { return signal_metrics; }
    
    // For convenience when checking direction
    bool isBuySignal() const { return direction == models::OrderSide::BUY; }
    bool isSellSignal() const { return direction == models::OrderSide::SELL; }
};

} // namespace execution
} // namespace trading