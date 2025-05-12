/**
 * @file order_book_analyzer.hpp
 * @brief Order book analysis component for trading strategies
 * 
 * This component analyzes order book data to extract key metrics and
 * identify patterns that may indicate profitable trading opportunities,
 * particularly focused on order imbalances.
 * 
 * @author Trading System Team
 * @date May 6, 2025
 */

 #pragma once

 #include <string>
 #include <vector>
 #include <map>
 #include <deque>
 #include <mutex>
 #include <memory>
 #include <functional>
 #include "strategy_types.hpp"
//  #include "strategy/types/types.hpp"
 #include "data_ingestion/orderbook/order_book.hpp"
 #include "data_ingestion/types.hpp"
 
 namespace trading {
 namespace strategy {
 
 /**
  * Order book metrics calculated by the analyzer
  */
 struct OrderBookMetrics {
     std::string exchange_id;
     std::string trading_pair;
     Timestamp timestamp;
     uint64_t sequence_number;
     
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
 };
 
 /**
  * Callback type for metric updates
  */
 using MetricsCallback = std::function<void(const OrderBookMetrics&)>;
 
 /**
  * OrderBookAnalyzer processes order book data to extract trading signals
  * based on order book imbalances and other market microstructure patterns.
  */
 class OrderBookAnalyzer {
 public:
     /**
      * Creates an order book analyzer for a specific exchange and trading pair
      */
     OrderBookAnalyzer(const std::string& exchange_id, const std::string& trading_pair, 
                      const ParameterSet& params = ParameterSet());
     
     /**
      * Creates an order book analyzer with parameters explicitly specified
      */
     OrderBookAnalyzer(const std::string& exchange_id, const std::string& trading_pair, 
                      int depth_levels, double price_grouping_factor);
     
     virtual ~OrderBookAnalyzer() = default;
     
     /**
      * Process an order book snapshot
      * 
      * @param snapshot Order book snapshot to analyze
      */
     virtual void processOrderBook(const data_ingestion::OrderBook& snapshot);
     
     /**
      * Get current order book metrics
      * 
      * @return Order book metrics
      */
     virtual OrderBookMetrics getMetrics() const;
     
     /**
      * Get a specific metric value
      * 
      * @param type Metric type
      * @param depth Depth level for level-specific metrics
      * @return Metric value
      */
     virtual double getMetric(const std::string& metric_name, int depth = 10) const;
     
     /**
      * Register a callback for metrics updates
      * 
      * @param callback Function to call when metrics are updated
      */
     virtual void registerMetricsCallback(MetricsCallback callback);
     
     /**
      * Check if enough data has been processed for reliable metrics
      * 
      * @return True if metrics are stable, false otherwise
      */
     virtual bool hasStableMetrics() const;
     
     /**
      * Reset the analyzer state
      */
     virtual void reset();
     
     /**
      * Set the depth of analysis
      * 
      * @param depth_levels Number of price levels to analyze
      */
     virtual void setDepthLevels(int depth_levels);
     
     /**
      * Set price grouping factor
      * 
      * @param factor Price grouping factor
      */
     virtual void setPriceGroupingFactor(double factor);
     
     /**
      * Get the exchange ID
      * @return Exchange ID
      */
     const std::string& getExchangeId() const { return exchange_id_; }
     
     /**
      * Get the trading pair
      * @return Trading pair
      */
     const std::string& getTradingPair() const { return trading_pair_; }
     
 protected:
     /**
      * Calculate basic price metrics
      * 
      * @param bids Bid price levels
      * @param asks Ask price levels
      */
     virtual void calculatePriceMetrics(
         const std::vector<data_ingestion::PriceLevel>& bids,
         const std::vector<data_ingestion::PriceLevel>& asks);
     
     /**
      * Calculate volume-based metrics
      * 
      * @param bids Bid price levels
      * @param asks Ask price levels
      * @param depth Depth level to consider
      */
     virtual void calculateVolumeMetrics(
         const std::vector<data_ingestion::PriceLevel>& bids,
         const std::vector<data_ingestion::PriceLevel>& asks,
         int depth);
     
     /**
      * Calculate metrics for each price level
      * 
      * @param bids Bid price levels
      * @param asks Ask price levels
      */
     virtual void calculateLevelMetrics(
         const std::vector<data_ingestion::PriceLevel>& bids,
         const std::vector<data_ingestion::PriceLevel>& asks);
     
     /**
      * Calculate imbalance metrics
      * 
      * @param bids Bid price levels
      * @param asks Ask price levels
      */
     virtual void calculateImbalanceMetrics(
         const std::vector<data_ingestion::PriceLevel>& bids,
         const std::vector<data_ingestion::PriceLevel>& asks);
     
     /**
      * Calculate derived metrics based on time series data
      */
     virtual void calculateDerivedMetrics();
     
     /**
      * Update the time series data
      */
     virtual void updateTimeSeries();
     
     /**
      * Calculate time-based metrics (velocities, accelerations)
      */
     virtual void calculateTimeBasedMetrics();
     
     /**
      * Process level data with price grouping
      * 
      * @param levels Original price levels
      * @param is_bids Whether these are bid levels
      * @return Grouped price levels
      */
     virtual std::vector<data_ingestion::PriceLevel> groupPriceLevels(
         const std::vector<data_ingestion::PriceLevel>& levels,
         bool is_bids) const;
     
     /**
      * Calculate weighted imbalance metric
      * 
      * @param bids Bid price levels
      * @param asks Ask price levels
      * @param weighting_function Function to weight each level
      * @return Weighted imbalance
      */
     virtual double calculateWeightedImbalance(
         const std::vector<data_ingestion::PriceLevel>& bids,
         const std::vector<data_ingestion::PriceLevel>& asks,
         const std::function<double(int)>& weighting_function) const;
     
     /**
      * Detect liquidity walls (large orders at specific levels)
      * 
      * @param levels Price levels to analyze
      * @param threshold Volume threshold factor
      * @return Vector of indices with liquidity walls
      */
     virtual std::vector<int> detectLiquidityWalls(
         const std::vector<data_ingestion::PriceLevel>& levels,
         double threshold = 3.0) const;
     
     /**
      * Calculate market directional coefficient
      * 
      * @return Directional coefficient (-1 to 1)
      */
     virtual double calculateDirectionalCoefficient() const;
     
     /**
      * Calculate volatility expectation based on order book structure
      * 
      * @return Volatility expectation
      */
     virtual double calculateVolatilityExpectation() const;
     
     /**
      * Notify callbacks of metrics update
      */
     virtual void notifyMetricsUpdated();
     
     // Exchange and trading pair identification
     std::string exchange_id_;
     std::string trading_pair_;
     
     // Configuration
     int depth_levels_;                    // Number of price levels to analyze
     double price_grouping_factor_;        // Factor for grouping close prices
     
     // State
     OrderBookMetrics current_metrics_;
     std::deque<OrderBookMetrics> metrics_history_;
     int min_samples_for_stability_;        // Minimum samples for stable metrics
     
     // Callbacks
     std::vector<MetricsCallback> metrics_callbacks_;
     
     // Thread safety
     mutable std::mutex mutex_;
     
     // Historical data
     std::vector<double> historical_mid_prices_;
     std::vector<double> historical_spreads_;
     std::vector<double> historical_volumes_;
     std::vector<double> historical_imbalances_;
     
     // Timestamps for rate calculations
     Timestamp last_update_time_;
     
     // Moving averages for smoothing
     double ema_mid_price_ = 0.0;
     double ema_spread_ = 0.0;
     double ema_volume_imbalance_ = 0.0;
     double ema_alpha_ = 0.2;              // Smoothing factor for EMAs
 };
 
 // Factory function to create OrderBookAnalyzer instances
 std::shared_ptr<OrderBookAnalyzer> createOrderBookAnalyzer(
     const std::string& exchange_id, 
     const std::string& trading_pair,
     const ParameterSet& params = ParameterSet());
 
 } // namespace strategy
 } // namespace trading