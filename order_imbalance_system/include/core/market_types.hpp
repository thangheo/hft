/**
 * @file market_types.hpp
 * @brief Core market-related types and utilities
 * 
 * This file defines common market structures used across all components
 * of the trading system.
 * 
 * @author Trading System Team
 * @date May 12, 2025
 */

 #pragma once

 #include <string>
 #include <cmath>
 #include <vector>
 #include <algorithm>
 namespace trading {
 namespace core {
 
 /**
  * Represents a trading symbol (trading pair)
  */
 struct Symbol {
     std::string base;    // Base currency (e.g., BTC)
     std::string quote;   // Quote currency (e.g., USDT)
     
     Symbol() = default;
     
     Symbol(const std::string& b, const std::string& q) : base(b), quote(q) {}
     
     // Create from combined string (e.g., "BTC/USDT")
     static Symbol fromString(const std::string& symbol_str) {
         size_t delimiter_pos = symbol_str.find('/');
         if (delimiter_pos != std::string::npos) {
             return Symbol(
                 symbol_str.substr(0, delimiter_pos),
                 symbol_str.substr(delimiter_pos + 1)
             );
         }
         // Alternative delimiter
         delimiter_pos = symbol_str.find('-');
         if (delimiter_pos != std::string::npos) {
             return Symbol(
                 symbol_str.substr(0, delimiter_pos),
                 symbol_str.substr(delimiter_pos + 1)
             );
         }
         // For symbols without delimiter, try to split at a reasonable position
         if (symbol_str.length() >= 6) { // BTCUSDT
             // Heuristic: last 3-4 characters are usually the quote
             size_t split = symbol_str.length() - (symbol_str.length() >= 7 ? 4 : 3);
             return Symbol(
                 symbol_str.substr(0, split),
                 symbol_str.substr(split)
             );
         }
         return Symbol(symbol_str, "");
     }
     
     // Convert to string representation
     std::string toString() const {
         return base + "/" + quote;
     }
     
     // Equality operator
     bool operator==(const Symbol& other) const {
         return base == other.base && quote == other.quote;
     }
     
     // Inequality operator
     bool operator!=(const Symbol& other) const {
         return !(*this == other);
     }
     
     // Less than operator (for containers)
     bool operator<(const Symbol& other) const {
         if (base != other.base) return base < other.base;
         return quote < other.quote;
     }
 };
 
 /**
  * Represents a price level in an order book
  */
 struct PriceLevel {
     double price;     // Price level
     double quantity;  // Quantity available at this price level
     
     PriceLevel() : price(0.0), quantity(0.0) {}
     
     PriceLevel(double p, double q) : price(p), quantity(q) {}
     
     // Equality operator
     bool operator==(const PriceLevel& other) const {
         // Use epsilon comparison for floating point values
         const double epsilon = 1e-9;
         return std::abs(price - other.price) < epsilon && 
                std::abs(quantity - other.quantity) < epsilon;
     }
     
     // Inequality operator
     bool operator!=(const PriceLevel& other) const {
         return !(*this == other);
     }
     
     // Less than operator (for containers)
     bool operator<(const PriceLevel& other) const {
         return price < other.price;
     }
 };
 
 /**
  * Represents a cryptocurrency exchange
  */
 enum class Exchange {
     BINANCE,
     COINBASE,
     FTX,
     KRAKEN,
     HUOBI,
     UNKNOWN
 };
 
 /**
  * Converts Exchange enum to string
  */
 inline std::string exchangeToString(Exchange exchange) {
     switch (exchange) {
         case Exchange::BINANCE:  return "binance";
         case Exchange::COINBASE: return "coinbase";
         case Exchange::FTX:      return "ftx";
         case Exchange::KRAKEN:   return "kraken";
         case Exchange::HUOBI:    return "huobi";
         case Exchange::UNKNOWN:  return "unknown";
         default:                 return "unknown";
     }
 }
 
 /**
  * Converts string to Exchange enum
  */
 inline Exchange exchangeFromString(const std::string& str) {
     auto lower = str;
     std::transform(lower.begin(), lower.end(), lower.begin(), 
         [](unsigned char c) { return std::tolower(c); });
     
     if (lower == "binance")  return Exchange::BINANCE;
     if (lower == "coinbase") return Exchange::COINBASE;
     if (lower == "ftx")      return Exchange::FTX;
     if (lower == "kraken")   return Exchange::KRAKEN;
     if (lower == "huobi")    return Exchange::HUOBI;
     return Exchange::UNKNOWN;
 }
 
 /**
  * Common identifiers for strategy, signals, and other components
  */
 using StrategyId = std::string;
 using SignalId = std::string;
 using ExperimentId = std::string;
 using OrderId = std::string;
 
 } // namespace core
 } // namespace trading