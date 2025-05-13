/**
 * @file momentum_correlation.hpp
 * @brief Correlates order book imbalances with price movements
 * 
 * @details
 * This component analyzes the correlation between detected order book imbalances
 * and subsequent price movements to improve signal quality and prediction accuracy.
 * 
 * Created on: May 6, 2025
 */
#pragma once

#include "strategy/types.hpp"
#include <map>
#include <deque>
#include <chrono>
#include <vector>

namespace trading {
namespace strategy {

/**
 * @class MomentumCorrelation
 * @brief Correlates order book imbalances with price movements
 */
class MomentumCorrelation {
public:
    /**
     * @brief Constructor
     * @param window_size Size of the rolling window for correlation analysis
     */
    explicit MomentumCorrelation(size_t window_size = 100);
    
    /**
     * @brief Destructor
     */
    virtual ~MomentumCorrelation() = default;
    
    /**
     * @brief Update the correlation model with new data
     * @param metrics Order book metrics
     * @param movement Subsequent price movement
     */
    virtual void updateCorrelationModel(const OrderBookMetrics& metrics, const PriceMovement& movement);
    
    /**
     * @brief Predict price movement based on an imbalance event
     * @param event The imbalance event to analyze
     * @return Predicted price movement
     */
    virtual double predictPriceMovement(const ImbalanceEvent& event);
    
    /**
     * @brief Get correlation statistics for all tracked metrics
     * @return Structure containing correlation statistics
     */
    virtual CorrelationStats getCorrelationStats() const;
    
    /**
     * @brief Configure the correlation parameters
     * @param params Correlation configuration parameters
     */
    void configure(const CorrelationParameters& params);
    
    /**
     * @brief Reset correlation data and start fresh
     */
    void reset();
    
    /**
     * @brief Detect the current market regime
     * @return Identified market regime
     */
    MarketRegime detectMarketRegime() const;
    
    /**
     * @brief Get correlation coefficient for a specific metric
     * @param metric_name Name of the metric
     * @return Correlation coefficient (-1.0 to 1.0)
     */
    double getCorrelationCoefficient(const std::string& metric_name) const;
    
    /**
     * @brief Get feature importance for imbalance metrics
     * @return Map of metric names to importance scores
     */
    std::map<std::string, double> getFeatureImportance() const;

private:
    size_t window_size_;
    CorrelationParameters params_;
    
    // Data storage for correlation analysis
    struct DataPoint {
        OrderBookMetrics metrics;
        PriceMovement price_movement;
        Timestamp timestamp;
    };
    
    std::deque<DataPoint> data_points_;
    std::map<std::string, double> correlation_coefficients_;
    std::map<std::string, double> feature_importance_;
    MarketRegime current_regime_ = MarketRegime::NORMAL;
    
    /**
     * @brief Calculate correlation coefficients from stored data points
     */
    void calculateCorrelations();
    
    /**
     * @brief Calculate feature importance using statistical methods
     */
    void calculateFeatureImportance();
    
    /**
     * @brief Check if the correlation is statistically significant
     * @param coefficient The correlation coefficient
     * @param sample_size Number of samples used
     * @return True if statistically significant
     */
    bool isStatisticallySignificant(double coefficient, size_t sample_size) const;
    
    /**
     * @brief Detect changes in market regime from correlation patterns
     */
    void updateMarketRegime();
    
    /**
     * @brief Apply weights to different metrics based on their predictive power
     * @param event The imbalance event to evaluate
     * @return Weighted prediction value
     */
    double calculateWeightedPrediction(const ImbalanceEvent& event) const;
};

} // namespace strategy
} // namespace trading