/**
 * @file imbalance_signal_generator.hpp
 * @brief Generates trading signals from detected order book imbalances
 * 
 * @details
 * This component is responsible for converting detected imbalance patterns
 * into actionable trading signals with direction, strength, and confidence levels.
 * 
 * Created on: May 6, 2025
 */
#pragma once

#include "strategy/types.hpp"
#include <vector>
#include <functional>
#include <memory>

namespace trading {
namespace strategy {

/**
 * @class ImbalanceSignalGenerator
 * @brief Generates trading signals from detected order book imbalances
 */
class ImbalanceSignalGenerator {
public:
    /**
     * @brief Constructor
     */
    ImbalanceSignalGenerator();
    
    /**
     * @brief Destructor
     */
    virtual ~ImbalanceSignalGenerator() = default;
    
    /**
     * @brief Process newly detected imbalances
     * @param imbalances Vector of imbalance events to process
     */
    virtual void processImbalances(const std::vector<ImbalanceEvent>& imbalances);
    
    /**
     * @brief Generate trading signals from processed imbalances
     * @return Vector of trading signals
     */
    virtual std::vector<TradingSignal> generateSignals();
    
    /**
     * @brief Configure signal generation parameters
     * @param params Signal generation parameters
     */
    virtual void configureSignalParameters(const SignalParameters& params);
    
    /**
     * @brief Register a callback for new signals
     * @param callback Function to call when new signals are generated
     */
    virtual void registerSignalCallback(const SignalCallback& callback);
    
    /**
     * @brief Clear existing imbalance data
     */
    virtual void clear();
    
    /**
     * @brief Set the momentum correlation component for enhanced signals
     * @param momentum_correlation Pointer to momentum correlation component
     */
    void setMomentumCorrelation(std::shared_ptr<class MomentumCorrelation> momentum_correlation);
    
    /**
     * @brief Set the threshold manager for dynamic signal adjustment
     * @param threshold_manager Pointer to threshold manager component
     */
    void setThresholdManager(std::shared_ptr<class ThresholdManager> threshold_manager);
    
    /**
     * @brief Set the signal decay model for time-based signal adjustment
     * @param signal_decay_model Pointer to signal decay model
     */
    void setSignalDecayModel(std::shared_ptr<class SignalDecayModel> signal_decay_model);

private:
    std::vector<ImbalanceEvent> pending_imbalances_;
    SignalParameters signal_params_;
    SignalCallback signal_callback_;
    std::shared_ptr<class MomentumCorrelation> momentum_correlation_;
    std::shared_ptr<class ThresholdManager> threshold_manager_;
    std::shared_ptr<class SignalDecayModel> signal_decay_model_;
    
    /**
     * @brief Convert an imbalance event to a trading signal
     * @param event The imbalance event to convert
     * @return The generated trading signal
     */
    TradingSignal createSignalFromImbalance(const ImbalanceEvent& event);
    
    /**
     * @brief Calculate signal strength based on imbalance metrics
     * @param event The imbalance event
     * @return Signal strength value between 0.0 and 1.0
     */
    double calculateSignalStrength(const ImbalanceEvent& event);
    
    /**
     * @brief Calculate signal confidence based on historical accuracy
     * @param event The imbalance event
     * @return Confidence level between 0.0 and 1.0
     */
    double calculateConfidence(const ImbalanceEvent& event);
    
    /**
     * @brief Determine appropriate time-to-live for the signal
     * @param event The imbalance event
     * @return Time-to-live in milliseconds
     */
    uint64_t determineSignalTTL(const ImbalanceEvent& event);
    
    /**
     * @brief Resolve conflicts between contrary signals
     * @param signals Vector of potentially conflicting signals
     * @return Vector of resolved signals
     */
    std::vector<TradingSignal> resolveSignalConflicts(const std::vector<TradingSignal>& signals);
};

} // namespace strategy
} // namespace trading