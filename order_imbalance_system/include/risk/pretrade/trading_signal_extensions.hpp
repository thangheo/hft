#pragma once

#include "execution/trading_signal.hpp"

namespace trading::risk {

// Use the execution TradingSignal
using TradingSignal = execution::TradingSignal;

// Extension functions
inline core::ExchangeId getExchangeId(const TradingSignal& signal) noexcept {
    return signal.exchange;
}

inline std::string getSymbol(const TradingSignal& signal) noexcept {
    return signal.getTradingPair();
}

inline double getQuantity(const TradingSignal& signal) noexcept {
    return signal.getTargetSize();
}

inline std::string getStrategyId(const TradingSignal& signal) noexcept {
    return signal.getStrategyId();
}

inline std::string getBaseAsset(const TradingSignal& signal) noexcept {
    const std::string& pair = signal.getTradingPair();
    const auto delimiter = pair.find('/');
    if (delimiter != std::string::npos) {
        return pair.substr(0, delimiter);
    }
    return pair.length() >= 6 ? pair.substr(0, 3) : pair;
}

} // namespace trading::risk