/**
 * Common type definitions used across risk management modules
 *
 * File: risk_types.hpp
 * Created: 2025-04-29
 */

#pragma once

#include "core/market_types.hpp"
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
// class MarketData;

/**
 * Type for timestamp representation
 */
using Timestamp = std::chrono::time_point<std::chrono::system_clock>;
using MarketDataUpdate = core::MarketDataUpdate;    
using MarketData = core::MarketData;
/**
 * Structure to hold market data information
 */
// struct MarketData {
//     std::string tradingPair;              // Trading pair symbol (e.g., "BTC/USDT")
//     std::string exchangeId;               // Exchange identifier
//     Timestamp timestamp;                  // Timestamp of the data
//     double price;                         // Current market price
//     double volume;                        // Trading volume
//     double bid;                           // Best bid price
//     double ask;                           // Best ask price
//     double lastTradePrice;                // Last trade price
//     double lastTradeQuantity;             // Last trade quantity
//     double open24h;                       // Opening price 24 hours ago
//     double high24h;                       // Highest price in last 24 hours
//     double low24h;                        // Lowest price in last 24 hours
//     double volumeQuote24h;                // 24-hour volume in quote currency
//     std::map<std::string, double> extra;  // Extra fields for extensibility
    
//     MarketData() 
//         : price(0.0), volume(0.0), bid(0.0), ask(0.0), 
//           lastTradePrice(0.0), lastTradeQuantity(0.0),
//           open24h(0.0), high24h(0.0), low24h(0.0), volumeQuote24h(0.0) {}
// };

/**
 * Enum for order side
 */
enum class OrderSide {
    BUY,
    SELL
};

/**
 * Order request structure
 */
struct OrderRequest {
    std::string orderId;
    std::string clientOrderId;
    std::string exchangeId;
    std::string tradingPair;
    OrderSide side;
    std::string type;
    double quantity;
    double price;
    std::string timeInForce;
    std::string strategyId;
    std::string signalId;
    Timestamp timestamp;
    std::map<std::string, std::string> params;
};

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
