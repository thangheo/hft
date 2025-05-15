/**
 * @file momentum_correlation.cpp
 * @brief Implementation of order book imbalance to price movement correlation
 */

 #include "strategy/signal/momentum_correlation.hpp"
    #include "strategy/types/constants.hpp"
 #include <algorithm>
 #include <cmath>
 #include <numeric>
 #include <iostream>
 
 namespace trading {
 namespace strategy {
 
 MomentumCorrelation::MomentumCorrelation(size_t window_size)
     : window_size_(window_size) {
     // Initialize correlation parameters with defaults
     params_.min_data_points = 10;
     params_.significance_threshold = 0.05;
     params_.correlation_decay_rate = 0.95;
     params_.regime_detection_window = 20;
 }
 
 void MomentumCorrelation::updateCorrelationModel(
     const OrderBookMetrics& metrics, const PriceMovement& movement) {
     
     // Create a new data point
     DataPoint dataPoint;
     dataPoint.metrics = metrics;
     dataPoint.price_movement = movement;
     dataPoint.timestamp = metrics.timestamp;
     
     // Add to data points
     data_points_.push_back(dataPoint);
     
     // Maintain window size
     while (data_points_.size() > window_size_) {
         data_points_.pop_front();
     }
     
     // Only calculate correlations when we have enough data
     if (data_points_.size() >= params_.min_data_points) {
         calculateCorrelations();
         calculateFeatureImportance();
         updateMarketRegime();
     }
 }
 
 double MomentumCorrelation::predictPriceMovement(const ImbalanceEvent& event) {
     // If we don't have enough data, return zero
     if (data_points_.size() < params_.min_data_points) {
         return 0.0;
     }
     
     // Use weighted prediction based on correlation coefficients and feature importance
     return calculateWeightedPrediction(event);
 }
 
 CorrelationStats MomentumCorrelation::getCorrelationStats() const {
     CorrelationStats stats;
     stats.coefficients = correlation_coefficients_;
     stats.feature_importance = feature_importance_;
     stats.data_points = data_points_.size();
     stats.market_regime = current_regime_;
     
     // Calculate overall predictive power
     double sumImportance = 0.0;
     for (const auto& [feature, importance] : feature_importance_) {
         sumImportance += importance * std::abs(correlation_coefficients_.at(feature));
     }
     stats.overall_predictive_power = sumImportance;
     
     return stats;
 }
 
 void MomentumCorrelation::configure(const CorrelationParameters& params) {
     params_ = params;
 }
 
 void MomentumCorrelation::reset() {
     data_points_.clear();
     correlation_coefficients_.clear();
     feature_importance_.clear();
     current_regime_ = MarketRegime::NORMAL;
 }
 
 MarketRegime MomentumCorrelation::detectMarketRegime() const {
     return current_regime_;
 }
 
 double MomentumCorrelation::getCorrelationCoefficient(const std::string& metric_name) const {
     auto it = correlation_coefficients_.find(metric_name);
     if (it != correlation_coefficients_.end()) {
         return it->second;
     }
     return 0.0;
 }
 
 std::map<std::string, double> MomentumCorrelation::getFeatureImportance() const {
     return feature_importance_;
 }
//  updateParameters
    void MomentumCorrelation::updateParameters(const CorrelationParameters& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        params_ = params;
    }
 void MomentumCorrelation::calculateCorrelations() {
     // Extract key metrics for correlation
     std::map<std::string, std::vector<double>> metricValues;
     std::vector<double> priceMovements;
     
     for (const auto& dataPoint : data_points_) {
         // Add price movement
         priceMovements.push_back(dataPoint.price_movement.percent_change);
         
         // Extract and store key metrics
         // Bid/Ask ratio
         metricValues["bid_ask_ratio"].push_back(
             dataPoint.metrics.bid_volume / std::max(0.00001, dataPoint.metrics.ask_volume));
         
         // Volume imbalance
         metricValues["volume_imbalance"].push_back(
             (dataPoint.metrics.bid_volume - dataPoint.metrics.ask_volume) / 
             (dataPoint.metrics.bid_volume + dataPoint.metrics.ask_volume));
         
         // Spread and spread percent
         metricValues["spread"].push_back(dataPoint.metrics.spread);
         metricValues["spread_percent"].push_back(dataPoint.metrics.spread_percent);
         
         // Price level imbalance metrics
         for (size_t level = 1; level <= 5; ++level) {
             std::string levelKey = "level_" + std::to_string(level) + "_imbalance";
             if (dataPoint.metrics.level_imbalances.count(level) > 0) {
                 metricValues[levelKey].push_back(dataPoint.metrics.level_imbalances.at(level));
             }
         }
         
         // Add custom metrics if available
         for (const auto& [key, value] : dataPoint.metrics.custom_metrics) {
             metricValues[key].push_back(value);
         }
     }
     
     // Calculate correlation coefficient for each metric
     for (auto& [metric, values] : metricValues) {
         // Skip if we don't have enough data points
         if (values.size() != priceMovements.size() || values.size() < 3) {
             correlation_coefficients_[metric] = 0.0;
             continue;
         }
         
         // Calculate means
         double meanMetric = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
         double meanPrice = std::accumulate(priceMovements.begin(), priceMovements.end(), 0.0) / priceMovements.size();
         
         // Calculate covariance and variances
         double covariance = 0.0;
         double varianceMetric = 0.0;
         double variancePrice = 0.0;
         
         for (size_t i = 0; i < values.size(); ++i) {
             double metricDiff = values[i] - meanMetric;
             double priceDiff = priceMovements[i] - meanPrice;
             
             covariance += metricDiff * priceDiff;
             varianceMetric += metricDiff * metricDiff;
             variancePrice += priceDiff * priceDiff;
         }
         
         // Avoid division by zero
         if (varianceMetric < 1e-10 || variancePrice < 1e-10) {
             correlation_coefficients_[metric] = 0.0;
             continue;
         }
         
         // Calculate Pearson correlation coefficient
         double correlation = covariance / (std::sqrt(varianceMetric) * std::sqrt(variancePrice));
         
         // Decay existing correlation value if it exists
         if (correlation_coefficients_.count(metric) > 0) {
             correlation = params_.correlation_decay_rate * correlation_coefficients_[metric] + 
                           (1 - params_.correlation_decay_rate) * correlation;
         }
         
         correlation_coefficients_[metric] = correlation;
     }
 }
 
 void MomentumCorrelation::calculateFeatureImportance() {
     // Simple feature importance calculation based on correlation magnitude
     // In a more sophisticated implementation, this could be based on 
     // regression weights, mutual information, or other methods
     
     double totalImportance = 0.0;
     std::map<std::string, double> rawImportance;
     
     for (const auto& [metric, correlation] : correlation_coefficients_) {
         double absCorrelation = std::abs(correlation);
         // Only consider correlations that are statistically significant
         if (isStatisticallySignificant(correlation, data_points_.size())) {
             rawImportance[metric] = absCorrelation * absCorrelation; // Square to emphasize stronger correlations
             totalImportance += rawImportance[metric];
         } else {
             rawImportance[metric] = 0.0;
         }
     }
     
     // Normalize importance values
     if (totalImportance > 0.0) {
         for (auto& [metric, importance] : rawImportance) {
             feature_importance_[metric] = importance / totalImportance;
         }
     } else {
         // If no significant correlations, assign equal importance
         double equalImportance = 1.0 / std::max(1.0, static_cast<double>(correlation_coefficients_.size()));
         for (const auto& [metric, _] : correlation_coefficients_) {
             feature_importance_[metric] = equalImportance;
         }
     }
 }
 
 bool MomentumCorrelation::isStatisticallySignificant(double coefficient, size_t sample_size) const {
     // Calculate t-statistic for Pearson correlation
     double absCorrelation = std::abs(coefficient);
     
     // Critical value depends on significance level and degrees of freedom
     // This is a simplified approximation
     double criticalValue = 0.0;
     
     // Different critical values based on sample size
     if (sample_size <= 5) {
         criticalValue = 0.878; // p = 0.05, df = 3
     } else if (sample_size <= 10) {
         criticalValue = 0.632; // p = 0.05, df = 8
     } else if (sample_size <= 20) {
         criticalValue = 0.444; // p = 0.05, df = 18
     } else if (sample_size <= 50) {
         criticalValue = 0.279; // p = 0.05, df = 48
     } else if (sample_size <= 100) {
         criticalValue = 0.197; // p = 0.05, df = 98
     } else {
         criticalValue = 0.124; // p = 0.05, df = 250
     }
     
     // Adjust based on significance threshold parameter
     criticalValue *= (params_.significance_threshold / 0.05);
     
     return absCorrelation > criticalValue;
 }
 
 void MomentumCorrelation::updateMarketRegime() {
     // We need enough data points for reliable regime detection
     if (data_points_.size() < params_.regime_detection_window) {
         current_regime_ = MarketRegime::NORMAL;
         return;
     }
     
     // Get recent price movements
     std::vector<double> recentMovements;
     size_t startIdx = std::max(0, static_cast<int>(data_points_.size() - params_.regime_detection_window));
     
     for (size_t i = startIdx; i < data_points_.size(); ++i) {
         recentMovements.push_back(data_points_[i].price_movement.percent_change);
     }
     
     // Calculate volatility (standard deviation of price movements)
     double mean = std::accumulate(recentMovements.begin(), recentMovements.end(), 0.0) / recentMovements.size();
     double variance = 0.0;
     
     for (double movement : recentMovements) {
         variance += (movement - mean) * (movement - mean);
     }
     
     variance /= recentMovements.size();
     double volatility = std::sqrt(variance);
     
     // Calculate directional movement (absolute sum of price movements)
     double directionalMovement = std::abs(std::accumulate(recentMovements.begin(), recentMovements.end(), 0.0));
     
     // Calculate oscillation (sum of absolute movements)
     double oscillation = 0.0;
     for (double movement : recentMovements) {
         oscillation += std::abs(movement);
     }
     
     // Determine regime based on volatility and directional movement
     if (volatility > 0.003) { // High volatility threshold (0.3% per data point)
         current_regime_ = MarketRegime::HIGH_VOLATILITY;
     } else if (directionalMovement > 0.005 && oscillation > 0.01) { // Strong trend (0.5% total with oscillations)
         current_regime_ = MarketRegime::TRENDING;
     } else if (oscillation < 0.005) { // Low oscillation (less than 0.5% total movement)
         current_regime_ = MarketRegime::QUIET;
     } else if (directionalMovement < 0.002 && oscillation > 0.008) { // Range-bound (high oscillation, low net movement)
         current_regime_ = MarketRegime::RANGE_BOUND;
     } else {
         current_regime_ = MarketRegime::NORMAL;
     }
 }
 
 double MomentumCorrelation::calculateWeightedPrediction(const ImbalanceEvent& event) const {
     double prediction = 0.0;
     double totalWeight = 0.0;
     
     // Use imbalance metrics and their correlations to predict price movement
     for (const auto& [metric, value] : event.supportingMetrics) {
         // Skip metrics without correlation data
         if (correlation_coefficients_.count(metric) == 0 || feature_importance_.count(metric) == 0) {
             continue;
         }
         
         double correlation = correlation_coefficients_.at(metric);
         double importance = feature_importance_.at(metric);
         
         // Weight is based on correlation strength and feature importance
         double weight = std::abs(correlation) * importance;
         
         // Direction is based on correlation sign
         double contribution = value * correlation * weight;
         
         prediction += contribution;
         totalWeight += weight;
     }
     
     // Also use primary metric if available
     std::string primaryMetricName = ImbalanceTypeToString(event.type);
     if (correlation_coefficients_.count(primaryMetricName) > 0 && feature_importance_.count(primaryMetricName) > 0) {
         double correlation = correlation_coefficients_.at(primaryMetricName);
         double importance = feature_importance_.at(primaryMetricName);
         double weight = std::abs(correlation) * importance * 1.5; // Extra weight for primary metric
         
         prediction += event.primaryMetric * correlation * weight;
         totalWeight += weight;
     }
     
     // Normalize by total weight if possible
     if (totalWeight > 0.0) {
         prediction /= totalWeight;
     }
     
     // Apply strength as a scaling factor
     prediction *= event.strength;
     
     // Apply market regime adjustment
     switch (current_regime_) {
         case MarketRegime::HIGH_VOLATILITY:
             prediction *= 0.8; // Reduce prediction magnitude in high volatility
             break;
         case MarketRegime::TRENDING:
             prediction *= 1.2; // Increase prediction in trending markets
             break;
         case MarketRegime::RANGE_BOUND:
             prediction *= 0.7; // Reduce prediction in range-bound markets
             break;
         case MarketRegime::QUIET:
             prediction *= 0.5; // Reduce prediction in quiet markets
             break;
         case MarketRegime::NORMAL:
         default:
             // No adjustment
             break;
     }
     
     return prediction;
 }
 
 } // namespace strategy
 } // namespace trading