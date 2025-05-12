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
//  struct OrderBookMetrics {
//      core::ExchangeId exchange_id;                 // Exchange identifier
//      core::Symbol trading_pair;                    // Trading pair
//      core::Timestamp timestamp;                    // Timestamp of these metrics
//      std::map<std::string, double> metrics;        // Map of metric name to value
     
//      // Default constructor
//      OrderBookMetrics()
//          : exchange_id(""), trading_pair(core::Symbol()), timestamp(core::Timestamp()) {}
         
//      // Constructor with exchange and trading pair
//      OrderBookMetrics(const core::ExchangeId& exchange, const core::Symbol& pair)
//          : exchange_id(exchange), trading_pair(pair), timestamp(std::chrono::system_clock::now()) {}
     
//      // Helper methods to access common metrics
//      double getBidAskRatio(int depth = 10) const {
//          return getMetric("bid_ask_ratio_" + std::to_string(depth));
//      }
     
//      double getBidAskImbalance(int depth = 10) const {
//          return getMetric("bid_ask_imbalance_" + std::to_string(depth));
//      }
     
//      double getMarketDepth(int depth = 10) const {
//          return getMetric("market_depth_" + std::to_string(depth));
//      }
     
//      double getBidVolume(int depth = 10) const {
//          return getMetric("bid_volume_" + std::to_string(depth));
//      }
     
//      double getAskVolume(int depth = 10) const {
//          return getMetric("ask_volume_" + std::to_string(depth));
//      }
     
//      double getSpread() const {
//          return getMetric("spread");
//      }
     
//      double getMidPrice() const {
//          return getMetric("mid_price");
//      }
     
//      // Generic metric getter with default value
//      double getMetric(const std::string& name, double default_value = 0.0) const {
//          auto it = metrics.find(name);
//          return (it != metrics.end()) ? it->second : default_value;
//      }
     
//      // Add or update a metric
//      void setMetric(const std::string& name, double value) {
//          metrics[name] = value;
//      }
//  };
 
 /**
  * Order book metrics calculated by the analyzer
  */
 struct OrderBookMetrics {
    core::Timestamp timestamp;
    uint64_t sequence_number;
    core::ExchangeId exchange_id;
    core::Symbol trading_pair;
    // Price metrics
    double mid_price = 0.0;
    double spread = 0.0;
    double spread_percent = 0.0;
    
    // Volume metrics
    double total_bid_volume = 0.0;
    double total_ask_volume = 0.0;
    double bid_ask_volume_ratio = 1.0;
    
    // Depth metrics
    std::vector<double> bid_volumes_by_level;
    std::vector<double> ask_volumes_by_level;
    std::vector<double> cumulative_bid_volumes;
    std::vector<double> cumulative_ask_volumes;
    
    // Imbalance metrics
    double volume_imbalance = 0.0;            // Overall volume imbalance
    double weighted_volume_imbalance = 0.0;   // Price-weighted volume imbalance
    double depth_imbalance = 0.0;             // Imbalance in market depth
    double pressure_imbalance = 0.0;          // Combined pressure imbalance
    
    // Level metrics
    double bid_density = 0.0;                 // Density of orders on bid side
    double ask_density = 0.0;                 // Density of orders on ask side
    double price_level_imbalance = 0.0;       // Imbalance in number of price levels
    
    // Derived metrics
    double market_directional_coefficient = 0.0;  // Market direction indicator
    double liquidity_profile_score = 0.0;         // Score of overall liquidity
    double volatility_expectation = 0.0;          // Expected near-term volatility
    
    // Historical metrics
    double price_velocity = 0.0;              // Rate of price change
    double volume_velocity = 0.0;             // Rate of volume change
    double spread_velocity = 0.0;             // Rate of spread change
    
    // Time-series metrics
    std::map<std::string, std::vector<double>> time_series_metrics;
    
    // Get a specific metric by name
    double getMetric(const std::string& name) const {
        if (name == "mid_price") return mid_price;
        if (name == "spread") return spread;
        if (name == "spread_percent") return spread_percent;
        if (name == "total_bid_volume") return total_bid_volume;
        if (name == "total_ask_volume") return total_ask_volume;
        if (name == "bid_ask_volume_ratio") return bid_ask_volume_ratio;
        if (name == "volume_imbalance") return volume_imbalance;
        if (name == "weighted_volume_imbalance") return weighted_volume_imbalance;
        if (name == "depth_imbalance") return depth_imbalance;
        if (name == "pressure_imbalance") return pressure_imbalance;
        if (name == "bid_density") return bid_density;
        if (name == "ask_density") return ask_density;
        if (name == "price_level_imbalance") return price_level_imbalance;
        if (name == "market_directional_coefficient") return market_directional_coefficient;
        if (name == "liquidity_profile_score") return liquidity_profile_score;
        if (name == "volatility_expectation") return volatility_expectation;
        if (name == "price_velocity") return price_velocity;
        if (name == "volume_velocity") return volume_velocity;
        if (name == "spread_velocity") return spread_velocity;
        
        // Default case
        return 0.0;
    }
    
    // Convert to map for easy access
    std::map<std::string, double> toMap() const {
        std::map<std::string, double> result;
        result["mid_price"] = mid_price;
        result["spread"] = spread;
        result["spread_percent"] = spread_percent;
        result["total_bid_volume"] = total_bid_volume;
        result["total_ask_volume"] = total_ask_volume;
        result["bid_ask_volume_ratio"] = bid_ask_volume_ratio;
        result["volume_imbalance"] = volume_imbalance;
        result["weighted_volume_imbalance"] = weighted_volume_imbalance;
        result["depth_imbalance"] = depth_imbalance;
        result["pressure_imbalance"] = pressure_imbalance;
        result["bid_density"] = bid_density;
        result["ask_density"] = ask_density;
        result["price_level_imbalance"] = price_level_imbalance;
        result["market_directional_coefficient"] = market_directional_coefficient;
        result["liquidity_profile_score"] = liquidity_profile_score;
        result["volatility_expectation"] = volatility_expectation;
        result["price_velocity"] = price_velocity;
        result["volume_velocity"] = volume_velocity;
        result["spread_velocity"] = spread_velocity;
        return result;
    }
    // Keep the useful methods from both implementations
    double getMetric(const std::string& name, double default_value = 0.0) const;
    // std::map<std::string, double> toMap() const;
};

 } // namespace strategy
 } // namespace trading