/**
 * @file fat_finger_protection.hpp
 * @brief Modern C++20 implementation of fat finger protection
 * 
 * Prevents abnormally large or mispriced orders using advanced statistical
 * analysis and modern C++ features for optimal performance.
 * 
 * @author Trading System Team
 * @date May 28, 2025
 */

#pragma once

#include "../risk_types.hpp"
#include "../risk_config.hpp" 
#include "../interfaces/data_interface.hpp"
#include "trading_signal_extensions.hpp"
#include "common/logger.hpp"

#include <concepts>
#include <ranges>
#include <span>
#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <expected>
#include <coroutine>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>

namespace trading::risk {

// Modern C++20 concepts for type safety
template<typename T>
concept TradingPairLike = requires(T t) {
    { t.symbol } -> std::convertible_to<std::string>;
} || std::convertible_to<T, std::string>;

template<typename T>
concept OrderLike = requires(T t) {
    { t.quantity } -> std::convertible_to<double>;
    { t.price } -> std::convertible_to<double>;
};

/**
 * Enhanced statistical analyzer for order patterns
 */
class OrderPatternAnalyzer {
public:
    struct Statistics {
        double mean{0.0};
        double median{0.0};
        double stdDev{0.0};
        double q25{0.0};
        double q75{0.0};
        size_t sampleSize{0};
        
        [[nodiscard]] constexpr bool isValid() const noexcept {
            return sampleSize > 0 && std::isfinite(mean) && std::isfinite(stdDev);
        }
    };

    explicit OrderPatternAnalyzer(size_t maxHistorySize = 200) noexcept
        : maxHistorySize_(maxHistorySize) {}

    void addSample(double value) noexcept;
    [[nodiscard]] Statistics getStatistics() const noexcept;
    [[nodiscard]] double getAnomalyScore(double value) const noexcept;
    [[nodiscard]] bool isOutlier(double value, double thresholdSigmas = 3.0) const noexcept;

private:
    mutable std::shared_mutex mutex_;
    std::deque<double> samples_;
    size_t maxHistorySize_;
    mutable std::optional<Statistics> cachedStats_;
    mutable bool statsDirty_{true};

    [[nodiscard]] Statistics calculateStatistics() const noexcept;
};

/**
 * Modern violation result using std::expected
 */
enum class ViolationCode {
    NO_VIOLATION,
    PRICE_DEVIATION_EXCESSIVE,
    SIZE_ANOMALY_DETECTED,  
    NOTIONAL_VALUE_TOO_HIGH,
    MARKET_DATA_UNAVAILABLE,
    INSUFFICIENT_HISTORY
};

struct ViolationDetails {
    ViolationCode code;
    std::string message;
    double expectedValue{0.0};
    double actualValue{0.0};
    double severity{0.0}; // 0.0 to 1.0
};

using ValidationResult = std::expected<void, ViolationDetails>;

/**
 * High-performance fat finger protection with modern C++20 features
 */
class FatFingerProtection {
public:
    /**
     * Configuration for fat finger protection
     */
    struct Config {
        double priceDeviationThreshold{0.05};      // 5%
        double sizeAnomalyThreshold{3.0};          // 3 sigma
        double notionalValueThreshold{10000.0};    // $10k
        double minimumHistorySize{10.0};           // Min samples needed
        bool rejectHighNotionalOrders{false};
        bool enableProgressiveThresholds{true};
        std::chrono::seconds marketDataTimeout{5};
        
        [[nodiscard]] constexpr bool isValid() const noexcept {
            return priceDeviationThreshold > 0.0 && priceDeviationThreshold < 1.0 &&
                   sizeAnomalyThreshold > 0.0 &&
                   notionalValueThreshold > 0.0 &&
                   minimumHistorySize > 0.0;
        }
    };

    explicit FatFingerProtection(
        std::shared_ptr<DataInterface> dataInterface,
        Config config = {}
    ) noexcept;

    // Modern async initialization 
    [[nodiscard]] std::expected<void, std::string> initialize() noexcept;

    // Primary validation methods using concepts
    template<TradingPairLike TPair>
    [[nodiscard]] ValidationResult validateSignal(
        const TradingSignal& signal,
        const TPair& tradingPair
    ) noexcept;

    template<OrderLike TOrder, TradingPairLike TPair>
    [[nodiscard]] ValidationResult validateOrder(
        const TOrder& order,
        const TPair& tradingPair
    ) noexcept;

    // Record order for pattern learning
    template<OrderLike TOrder, TradingPairLike TPair>
    void recordOrder(const TOrder& order, const TPair& tradingPair) noexcept;

    // Configuration management
    [[nodiscard]] bool updateConfig(const Config& newConfig) noexcept;
    [[nodiscard]] const Config& getConfig() const noexcept { return config_; }

    // Analytics and introspection
    [[nodiscard]] std::optional<OrderPatternAnalyzer::Statistics> 
    getOrderStatistics(std::string_view symbol) const noexcept;

    [[nodiscard]] bool requiresAdditionalVerification(
        std::string_view symbol,
        double price,
        double quantity
    ) const noexcept;

private:
    std::shared_ptr<DataInterface> dataInterface_;
    Config config_;
    
    // Thread-safe order history with modern concurrency
    mutable std::shared_mutex historyMutex_;
    std::unordered_map<std::string, std::unique_ptr<OrderPatternAnalyzer>> orderAnalyzers_;
    
    // Atomic metrics for performance monitoring
    std::atomic<uint64_t> totalValidations_{0};
    std::atomic<uint64_t> rejectedOrders_{0};
    std::atomic<uint64_t> cacheHits_{0};

    // Helper methods
    [[nodiscard]] std::string normalizeSymbol(std::string_view symbol) const noexcept;
    
    [[nodiscard]] ValidationResult validatePriceDeviation(
        std::string_view symbol,
        double orderPrice
    ) noexcept;

    [[nodiscard]] ValidationResult validateSizeAnomaly(
        std::string_view symbol,
        double orderSize
    ) noexcept;

    [[nodiscard]] ValidationResult validateNotionalValue(
        std::string_view symbol,
        double price,
        double quantity
    ) noexcept;

    [[nodiscard]] std::expected<double, std::string> 
    getCurrentMarketPrice(std::string_view symbol) noexcept;

    [[nodiscard]] OrderPatternAnalyzer& getOrCreateAnalyzer(
        std::string_view symbol
    ) noexcept;
};

// Template implementations

template<TradingPairLike TPair>
ValidationResult FatFingerProtection::validateSignal(
    const TradingSignal& signal,
    const TPair& tradingPair
) noexcept {
    ++totalValidations_;
    
    const std::string symbol = [&]() {
        if constexpr (std::convertible_to<TPair, std::string>) {
            return std::string{tradingPair};
        } else {
            return tradingPair.symbol;
        }
    }();

    // For signals, we only validate size (no price available yet)
    return validateSizeAnomaly(symbol, signal.getTargetSize());
}

template<OrderLike TOrder, TradingPairLike TPair>
ValidationResult FatFingerProtection::validateOrder(
    const TOrder& order,
    const TPair& tradingPair
) noexcept {
    ++totalValidations_;
    
    const std::string symbol = [&]() {
        if constexpr (std::convertible_to<TPair, std::string>) {
            return std::string{tradingPair};
        } else {
            return tradingPair.symbol;
        }
    }();

    // Validate price deviation for limit orders
    if constexpr (requires { order.type; }) {
        if (order.type != "MARKET") {
            if (auto result = validatePriceDeviation(symbol, order.price); !result) {
                return result;
            }
        }
    }

    // Validate size anomaly
    if (auto result = validateSizeAnomaly(symbol, order.quantity); !result) {
        return result;
    }

    // Validate notional value
    return validateNotionalValue(symbol, order.price, order.quantity);
}

template<OrderLike TOrder, TradingPairLike TPair>
void FatFingerProtection::recordOrder(
    const TOrder& order,
    const TPair& tradingPair
) noexcept {
    const std::string symbol = [&]() {
        if constexpr (std::convertible_to<TPair, std::string>) {
            return std::string{tradingPair};
        } else {
            return tradingPair.symbol;
        }
    }();

    try {
        auto& analyzer = getOrCreateAnalyzer(symbol);
        analyzer.addSample(order.quantity);
        
        LOG_DEBUG("Recorded order for {}: size={}", symbol, order.quantity);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to record order for {}: {}", symbol, e.what());
    }
}

} // namespace trading::risk