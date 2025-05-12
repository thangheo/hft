/**
 * @file strategy/types/order_book_metrics.hpp
 * @brief Order book metrics structure
 * 
 * This file defines the OrderBookMetrics structure used throughout
 * the strategy component.
 * 
 * @author Trading System Team
 * @date May 12, 2025
 */

 #pragma once

 #include <string>
 #include <map>
 #include "core/types.hpp"
 #include "core/market_types.hpp"
 namespace trading {
 namespace strategy {
 
 /**
  * Represents a set of order book metrics
  */
 struct OrderBookMetrics {
     core::ExchangeId exchange_id;                 // Exchange identifier
     core::Symbol trading_pair;                    // Trading pair
     core::Timestamp timestamp;                    // Timestamp of these metrics
     std::map<std::string, double> metrics;        // Map of metric name to value
     
     // Default constructor
     OrderBookMetrics()
         : exchange_id(""), trading_pair(core::Symbol()), timestamp(core::Timestamp()) {}
         
     // Constructor with exchange and trading pair
     OrderBookMetrics(const core::ExchangeId& exchange, const core::Symbol& pair)
         : exchange_id(exchange), trading_pair(pair), timestamp(std::chrono::system_clock::now()) {}
     
     // Helper methods to access common metrics
     double getBidAskRatio(int depth = 10) const {
         return getMetric("bid_ask_ratio_" + std::to_string(depth));
     }
     
     double getBidAskImbalance(int depth = 10) const {
         return getMetric("bid_ask_imbalance_" + std::to_string(depth));
     }
     
     double getMarketDepth(int depth = 10) const {
         return getMetric("market_depth_" + std::to_string(depth));
     }
     
     double getBidVolume(int depth = 10) const {
         return getMetric("bid_volume_" + std::to_string(depth));
     }
     
     double getAskVolume(int depth = 10) const {
         return getMetric("ask_volume_" + std::to_string(depth));
     }
     
     double getSpread() const {
         return getMetric("spread");
     }
     
     double getMidPrice() const {
         return getMetric("mid_price");
     }
     
     // Generic metric getter with default value
     double getMetric(const std::string& name, double default_value = 0.0) const {
         auto it = metrics.find(name);
         return (it != metrics.end()) ? it->second : default_value;
     }
     
     // Add or update a metric
     void setMetric(const std::string& name, double value) {
         metrics[name] = value;
     }
 };
 
 } // namespace strategy
 } // namespace trading