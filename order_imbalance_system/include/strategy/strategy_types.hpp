/**
 * @file strategy_types.hpp
 * @brief Foundational types, enums, and constants for the Order Imbalance Strategy
 * 
 * This file defines the core data structures and types used throughout the strategy
 * component of the crypto trading system. It establishes common terminology and data
 * representations for order imbalance detection and signal generation.
 * 
 * @author Trading System Team
 * @date May 6, 2025
 */

 #pragma once

 #include <string>
 #include <vector>
 #include <map>
 #include <chrono>
 #include <memory>
 #include <functional>
 #include <optional>
 #include "common/types.hpp"
 
 namespace trading {
 namespace strategy {
 
 /**
  * Strategy component identification.
  */
 using StrategyId = std::string;
 using SignalId = std::string;
 using ExperimentId = std::string;
 
 /**
  * Time-related typedefs
  */
 using Timestamp = std::chrono::system_clock::time_point;
 using Duration = std::chrono::system_clock::duration;
 
 /**
  * Enum for direction of trading signals and imbalances
  */
 enum class Direction {
     BUY,
     SELL,
     NEUTRAL
 };
 
 /**
  * String conversion for Direction enum
  */
 inline std::string directionToString(Direction direction) {
     switch (direction) {
         case Direction::BUY: return "buy";
         case Direction::SELL: return "sell";
         case Direction::NEUTRAL: return "neutral";
         default: return "unknown";
     }
 }
 
 /**
  * Enum for different types of order book imbalances
  */
 enum class ImbalanceType {
     VOLUME_IMBALANCE,              // Significant difference between bid and ask volume
     PRICE_LEVEL_IMBALANCE,         // Imbalance across multiple price levels
     LIQUIDITY_REMOVAL,             // Sudden removal of liquidity from one side
     ORDER_FLOW_IMBALANCE,          // Directional bias in order submissions
     LARGE_ORDER_APPEARANCE,        // Large orders appearing in the book
     LARGE_ORDER_DISAPPEARANCE,     // Large orders being removed from the book
     PRICE_LEVEL_CONCENTRATION,     // Concentration of volume at specific price levels
     BID_ASK_SPREAD_CHANGE,         // Significant change in the bid-ask spread
     MARKET_DEPTH_CHANGE,           // Significant change in market depth
     CUSTOM                         // Custom imbalance type defined by strategy
 };
 
 /**
  * String conversion for ImbalanceType enum
  */
 inline std::string imbalanceTypeToString(ImbalanceType type) {
     switch (type) {
         case ImbalanceType::VOLUME_IMBALANCE: return "volume_imbalance";
         case ImbalanceType::PRICE_LEVEL_IMBALANCE: return "price_level_imbalance";
         case ImbalanceType::LIQUIDITY_REMOVAL: return "liquidity_removal";
         case ImbalanceType::ORDER_FLOW_IMBALANCE: return "order_flow_imbalance";
         case ImbalanceType::LARGE_ORDER_APPEARANCE: return "large_order_appearance";
         case ImbalanceType::LARGE_ORDER_DISAPPEARANCE: return "large_order_disappearance";
         case ImbalanceType::PRICE_LEVEL_CONCENTRATION: return "price_level_concentration";
         case ImbalanceType::BID_ASK_SPREAD_CHANGE: return "bid_ask_spread_change";
         case ImbalanceType::MARKET_DEPTH_CHANGE: return "market_depth_change";
         case ImbalanceType::CUSTOM: return "custom";
         default: return "unknown";
     }
 }
 
 /**
  * Level of confidence in detected imbalance or signal
  */
 enum class ConfidenceLevel {
     LOW,
     MEDIUM,
     HIGH,
     VERY_HIGH
 };
 
 /**
  * Represents a price target for a trading signal
  */
 struct PriceTarget {
     double price;                 // Target price level
     double probability;           // Probability of reaching this target
     Duration expected_time;       // Expected time to reach target
 };
 
 /**
  * Represents an imbalance event detected in the order book
  */
 struct ImbalanceEvent {
     ImbalanceType type;                                // Type of imbalance detected
     Direction direction;                               // Direction of imbalance (buy/sell)
     double strength;                                   // Normalized strength (0.0-1.0)
     Timestamp detection_time;                          // When the imbalance was detected
     double primary_metric;                             // Primary metric value for this imbalance
     std::map<std::string, double> supporting_metrics;  // Additional metrics supporting the imbalance
     uint64_t event_id;                                 // Unique identifier for this event
     ConfidenceLevel confidence;                        // Confidence level in this imbalance
 
     // Constructor with required fields
     ImbalanceEvent(ImbalanceType t, Direction d, double s, Timestamp dt)
         : type(t), direction(d), strength(s), detection_time(dt),
           primary_metric(0.0), event_id(0), confidence(ConfidenceLevel::MEDIUM) {}
     
     // Default constructor
     ImbalanceEvent() 
         : type(ImbalanceType::VOLUME_IMBALANCE), direction(Direction::NEUTRAL),
           strength(0.0), detection_time(Timestamp()), primary_metric(0.0),
           event_id(0), confidence(ConfidenceLevel::MEDIUM) {}
 };
 
 /**
  * Represents a trading signal generated from imbalance detection
  */
 struct TradingSignal {
     SignalId id;                                     // Unique identifier
     Direction direction;                             // Buy or sell
     double strength;                                 // Signal strength (0.0-1.0)
     double confidence;                               // Confidence level (0.0-1.0)
     Timestamp generation_time;                       // When the signal was generated
     uint64_t ttl;                                    // Time-to-live in milliseconds
     std::vector<PriceTarget> targets;                // Price targets
     ImbalanceType source_type;                       // Type of imbalance that generated this signal
     std::map<std::string, double> signal_metrics;    // Metrics associated with this signal
 
     // Constructor with required fields
     TradingSignal(SignalId sid, Direction d, double s, Timestamp gt)
         : id(sid), direction(d), strength(s), confidence(0.5), 
           generation_time(gt), ttl(0), source_type(ImbalanceType::VOLUME_IMBALANCE) {}
     
     // Default constructor
     TradingSignal()
         : id(""), direction(Direction::NEUTRAL), strength(0.0), confidence(0.0),
           generation_time(Timestamp()), ttl(0), source_type(ImbalanceType::VOLUME_IMBALANCE) {}
     
     // Check if signal is still valid based on TTL
     bool isValid(Timestamp current_time) const {
         auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
             current_time - generation_time).count();
         return elapsed < ttl;
     }
 
     // Calculate remaining strength based on signal decay
     double getRemainingStrength(Timestamp current_time, double decay_rate = 0.0) const {
         if (decay_rate <= 0.0) {
             return isValid(current_time) ? strength : 0.0;
         }
         
         auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
             current_time - generation_time).count();
         
         // Exponential decay formula: strength * e^(-decay_rate * elapsed_time)
         double decay_factor = std::exp(-decay_rate * elapsed_ms / 1000.0);
         return strength * decay_factor;
     }
 };
 
 /**
  * Represents a set of order book metrics
  */
 struct OrderBookMetrics {
     std::string exchange_id;                       // Exchange identifier
     std::string trading_pair;                      // Trading pair
     Timestamp timestamp;                           // Timestamp of these metrics
     std::map<std::string, double> metrics;         // Map of metric name to value
     
     // Helper methods to access common metrics
     double getBidAskRatio(int depth = 10) const {
         return getMetric("bid_ask_ratio_" + std::to_string(depth));
     }
     
     double getBidAskImbalance(int depth = 10) const {
         return getMetric("bid_ask_imbalance_" + std::to_string(depth));
     }
     
     double getMetric(const std::string& name) const {
         auto it = metrics.find(name);
         return (it != metrics.end()) ? it->second : 0.0;
     }
 
     // Default constructor
     OrderBookMetrics()
         : exchange_id(""), trading_pair(""), timestamp(Timestamp()) {}
         
     // Constructor with exchange and trading pair
     OrderBookMetrics(const std::string& exchange, const std::string& pair)
         : exchange_id(exchange), trading_pair(pair), timestamp(std::chrono::system_clock::now()) {}
 };
 
 /**
  * Represents a set of thresholds for imbalance detection
  */
 struct ThresholdSet {
     std::map<std::string, double> thresholds;     // Map of threshold name to value
     
     // Common threshold getters
     double getVolumeImbalanceThreshold() const {
         return getThreshold("volume_imbalance");
     }
     
     double getLiquidityRemovalThreshold() const {
         return getThreshold("liquidity_removal");
     }
     
     double getPriceLevelImbalanceThreshold() const {
         return getThreshold("price_level_imbalance");
     }
     
     // Generic threshold getter with default value
     double getThreshold(const std::string& name, double default_value = 0.0) const {
         auto it = thresholds.find(name);
         return (it != thresholds.end()) ? it->second : default_value;
     }
     
     // Threshold setter
     void setThreshold(const std::string& name, double value) {
         thresholds[name] = value;
     }
 };
 
 /**
  * Represents a set of parameters for the imbalance strategy
  */
 struct ParameterSet {
     std::string version;                              // Version identifier
     std::string exchange_id;                          // Exchange these parameters apply to
     Timestamp last_updated;                           // When parameters were last updated
     std::map<std::string, double> numeric_params;     // Numeric parameters
     std::map<std::string, std::string> string_params; // String parameters
     std::map<std::string, bool> bool_params;          // Boolean parameters
     std::map<std::string, std::vector<double>> array_params; // Array parameters
     
     // Parameter access helpers
     template<typename T>
     T getParameter(const std::string& name, T default_value) const;
     
     template<>
     double getParameter(const std::string& name, double default_value) const {
         auto it = numeric_params.find(name);
         return (it != numeric_params.end()) ? it->second : default_value;
     }
     
     template<>
     std::string getParameter(const std::string& name, std::string default_value) const {
         auto it = string_params.find(name);
         return (it != string_params.end()) ? it->second : default_value;
     }
     
     template<>
     bool getParameter(const std::string& name, bool default_value) const {
         auto it = bool_params.find(name);
         return (it != bool_params.end()) ? it->second : default_value;
     }
     
     template<>
     std::vector<double> getParameter(const std::string& name, std::vector<double> default_value) const {
         auto it = array_params.find(name);
         return (it != array_params.end()) ? it->second : default_value;
     }
     
     // Parameter setters
     void setParameter(const std::string& name, double value) {
         numeric_params[name] = value;
         last_updated = std::chrono::system_clock::now();
     }
     
     void setParameter(const std::string& name, const std::string& value) {
         string_params[name] = value;
         last_updated = std::chrono::system_clock::now();
     }
     
     void setParameter(const std::string& name, bool value) {
         bool_params[name] = value;
         last_updated = std::chrono::system_clock::now();
     }
     
     void setParameter(const std::string& name, const std::vector<double>& value) {
         array_params[name] = value;
         last_updated = std::chrono::system_clock::now();
     }
 };
 
 /**
  * Strategy variant for A/B testing
  */
 struct StrategyVariant {
     std::string variant_id;                 // Variant identifier
     ParameterSet parameters;                // Parameters for this variant
     std::string description;                // Description of this variant
     
     // Default constructor
     StrategyVariant()
         : variant_id(""), description("") {}
     
     // Constructor with variant ID
     StrategyVariant(const std::string& id)
         : variant_id(id), description("") {}
 };
 
 /**
  * Experiment definition for A/B testing
  */
 struct Experiment {
     ExperimentId id;                                  // Experiment identifier
     std::string description;                          // Description of the experiment
     std::vector<StrategyVariant> variants;            // Variants being tested
     Timestamp start_time;                             // Experiment start time
     std::optional<Timestamp> end_time;                // Experiment end time (if completed)
     std::map<std::string, double> traffic_allocation; // Traffic allocation to variants
     
     // Default constructor
     Experiment()
         : id(""), description(""), start_time(std::chrono::system_clock::now()) {}
     
     // Constructor with experiment ID
     Experiment(const ExperimentId& eid)
         : id(eid), description(""), start_time(std::chrono::system_clock::now()) {}
 };
 
 /**
  * Experiment results
  */
 struct ExperimentResults {
     ExperimentId experiment_id;                 // Experiment identifier
     std::map<std::string, double> metrics;      // Metrics for each variant
     std::string winning_variant;                // Identifier of winning variant (if any)
     double confidence_level;                    // Statistical confidence in the result
     
     // Default constructor
     ExperimentResults()
         : experiment_id(""), winning_variant(""), confidence_level(0.0) {}
     
     // Constructor with experiment ID
     ExperimentResults(const ExperimentId& eid)
         : experiment_id(eid), winning_variant(""), confidence_level(0.0) {}
 };
 
 /**
  * Performance metrics for strategy evaluation
  */
 struct PerformanceMetrics {
     double pnl;                         // Profit and loss
     double sharpe_ratio;                // Sharpe ratio
     double max_drawdown;                // Maximum drawdown
     double win_rate;                    // Win rate (percentage)
     double profit_factor;               // Profit factor (gross profit / gross loss)
     int trade_count;                    // Number of trades
     double avg_trade_pnl;               // Average P&L per trade
     
     // Default constructor with zeroed metrics
     PerformanceMetrics()
         : pnl(0.0), sharpe_ratio(0.0), max_drawdown(0.0), 
           win_rate(0.0), profit_factor(0.0), trade_count(0), avg_trade_pnl(0.0) {}
 };
 
 /**
  * Market state information
  */
 struct MarketState {
     std::string exchange_id;               // Exchange identifier
     std::string trading_pair;              // Trading pair
     Timestamp timestamp;                   // Timestamp of this state
     double price;                          // Current price
     double volatility;                     // Current volatility
     double volume_24h;                     // 24-hour volume
     double price_change_24h;               // 24-hour price change percentage
     std::string market_regime;             // Market regime classification
     
     // Default constructor
     MarketState()
         : exchange_id(""), trading_pair(""), timestamp(std::chrono::system_clock::now()),
           price(0.0), volatility(0.0), volume_24h(0.0), price_change_24h(0.0),
           market_regime("unknown") {}
     
     // Constructor with exchange and trading pair
     MarketState(const std::string& exchange, const std::string& pair)
         : exchange_id(exchange), trading_pair(pair), timestamp(std::chrono::system_clock::now()),
           price(0.0), volatility(0.0), volume_24h(0.0), price_change_24h(0.0),
           market_regime("unknown") {}
 };
 
 /**
  * Common callbacksfor strategy components
  */
 using ImbalanceCallback = std::function<void(const ImbalanceEvent&)>;
 using SignalCallback = std::function<void(const TradingSignal&)>;
 using MetricsCallback = std::function<void(const OrderBookMetrics&)>;
 
 // Forward declarations for important strategy classes
 class OrderBookAnalyzer;
 class ImbalanceDetectionEngine;
 class ImbalanceSignalGenerator;
 class StrategyEngine;
 
 // Shared pointers for strategy components
 using OrderBookAnalyzerPtr = std::shared_ptr<OrderBookAnalyzer>;
 using ImbalanceDetectionEnginePtr = std::shared_ptr<ImbalanceDetectionEngine>;
 using ImbalanceSignalGeneratorPtr = std::shared_ptr<ImbalanceSignalGenerator>;
 using StrategyEnginePtr = std::shared_ptr<StrategyEngine>;
 
 /**
  * Constants used throughout the strategy component
  */
 namespace constants {
     // Default thresholds
     constexpr double DEFAULT_VOLUME_IMBALANCE_THRESHOLD = 0.7;
     constexpr double DEFAULT_PRICE_LEVEL_IMBALANCE_THRESHOLD = 0.6;
     constexpr double DEFAULT_LIQUIDITY_REMOVAL_THRESHOLD = 0.5;
     
     // Default signal parameters
     constexpr double DEFAULT_SIGNAL_TTL_MS = 5000;  // 5 seconds
     constexpr double DEFAULT_SIGNAL_DECAY_RATE = 0.1;
     
     // Default depth values
     constexpr int DEFAULT_ORDER_BOOK_DEPTH = 10;
     constexpr int MAX_ORDER_BOOK_DEPTH = 50;
     
     // Signal strength thresholds
     constexpr double MIN_SIGNAL_STRENGTH = 0.1;
     constexpr double STRONG_SIGNAL_THRESHOLD = 0.7;
     
     // Strategy version
     const std::string STRATEGY_VERSION = "1.0.0";
 }
 
 } // namespace strategy
 } // namespace trading