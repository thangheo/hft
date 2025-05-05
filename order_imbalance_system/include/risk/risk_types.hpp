/**
 * Common type definitions used across risk management modules
 *
 * File: risk_types.hpp
 * Created: 2025-04-29
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <cstdint>

namespace trading {
namespace risk {

// Forward declarations
class Position;
class TradingSignal;
class OrderRequest;
class ValidationResult;
class ThresholdSet;
class RiskParameters;
class PerformanceMetrics;
class MarketData;

/**
 * Enum for different types of risk violations
 */
enum class ViolationType {
    POSITION_LIMIT,
    EXPOSURE_LIMIT,
    ORDER_SIZE_LIMIT,
    PRICE_DEVIATION,
    DRAWDOWN_LIMIT,
    RATE_LIMIT,
    COLLATERAL_REQUIREMENT,
    EXCHANGE_CONSTRAINT,
    OTHER
};

/**
 * Enum for risk manager mode/state
 */
enum class RiskManagerMode {
    NORMAL,
    HEIGHTENED_RISK,
    EMERGENCY,
    MAINTENANCE
};

/**
 * Enum for circuit breaker state
 */
enum class CircuitBreakerState {
    OFF,
    WARNING,
    TRIGGERED,
    COOLING
};

/**
 * Enum for trading direction
 */
enum class Direction {
    BUY,
    SELL
};

/**
 * Enum for order type
 */
enum class OrderType {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT,
    TAKE_PROFIT,
    TAKE_PROFIT_LIMIT,
    TRAILING_STOP
};

/**
 * Struct to represent a risk violation
 */
struct RiskViolation {
    ViolationType type;
    std::string description;
    double limitValue;
    double actualValue;
};

/**
 * Struct for exchange identification
 */
struct ExchangeId {
    std::string id;
    
    bool operator==(const ExchangeId& other) const {
        return id == other.id;
    }
    
    bool operator<(const ExchangeId& other) const {
        return id < other.id;
    }
};

/**
 * Struct for trading pair (symbol)
 */
struct TradingPair {
    std::string base;
    std::string quote;
    std::string symbol;
    
    bool operator==(const TradingPair& other) const {
        return symbol == other.symbol;
    }
    
    bool operator<(const TradingPair& other) const {
        return symbol < other.symbol;
    }
};

/**
 * Struct for strategy identification
 */
struct StrategyId {
    std::string id;
    std::string version;
    
    bool operator==(const StrategyId& other) const {
        return id == other.id && version == other.version;
    }
    
    bool operator<(const StrategyId& other) const {
        return (id == other.id) ? (version < other.version) : (id < other.id);
    }
};

/**
 * Type for timestamp representation
 */
using Timestamp = std::chrono::time_point<std::chrono::system_clock>;

/**
 * Custom exceptions for Risk Management System
 */
class RiskException : public std::runtime_error {
public:
    explicit RiskException(const std::string& message) : std::runtime_error(message) {}
};

class ValidationException : public RiskException {
public:
    explicit ValidationException(const std::string& message) : RiskException(message) {}
};

class ConfigurationException : public RiskException {
public:
    explicit ConfigurationException(const std::string& message) : RiskException(message) {}
};

class DataException : public RiskException {
public:
    explicit DataException(const std::string& message) : RiskException(message) {}
};

} // namespace risk
}
