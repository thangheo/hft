/**
 * @file signal_decay_model.hpp
 * @brief Models decay of trading signal relevance over time
 * 
 * @details
 * This component implements various decay models to represent how
 * trading signals lose strength and relevance over time.
 * 
 * Created on: May 6, 2025
 */
#pragma once

#include "strategy/strategy_types.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <memory>

namespace trading {
namespace strategy {

/**
 * @brief Enumeration of available decay model types
 */
enum class DecayModelType {
    EXPONENTIAL,      // Exponential decay function
    LINEAR,           // Linear decay function
    STEP,             // Step-wise decay function
    CONDITIONAL,      // Conditional decay based on market events
    CUSTOM            // Custom decay function
};

/**
 * @class SignalDecayModel
 * @brief Models how trading signals lose strength over time
 */
class SignalDecayModel {
public:
    /**
     * @brief Constructor
     * @param model_type The type of decay model to use
     */
    explicit SignalDecayModel(DecayModelType model_type = DecayModelType::EXPONENTIAL);
    
    /**
     * @brief Destructor
     */
    virtual ~SignalDecayModel() = default;
    
    /**
     * @brief Configure the decay parameters
     * @param params Decay configuration parameters
     */
    virtual void configureDecayParameters(const DecayParameters& params);
    
    /**
     * @brief Calculate remaining signal strength based on time elapsed
     * @param signal The original trading signal
     * @param current_time Current timestamp
     * @return Remaining signal strength (0.0-1.0)
     */
    virtual double calculateRemainingStrength(const TradingSignal& signal, uint64_t current_time);
    
    /**
     * @brief Check if a signal is still valid
     * @param signal The trading signal to check
     * @param current_time Current timestamp
     * @return True if the signal is still valid
     */
    virtual bool isSignalValid(const TradingSignal& signal, uint64_t current_time);
    
    /**
     * @brief Set a custom decay function
     * @param decay_func Custom function that calculates remaining strength
     */
    void setCustomDecayFunction(std::function<double(const TradingSignal&, uint64_t)> decay_func);
    
    /**
     * @brief Get the current decay model type
     * @return The decay model type
     */
    DecayModelType getModelType() const;
    
    /**
     * @brief Set the decay model type
     * @param model_type The decay model to use
     */
    void setModelType(DecayModelType model_type);
    
    /**
     * @brief Register a market event that could invalidate signals
     * @param event_type Type of market event
     * @param event_data Additional event data
     */
    void registerMarketEvent(MarketEventType event_type, const MarketEventData& event_data);
    
    /**
     * @brief Calibrate decay rates based on historical signal performance
     * @param signal_history Vector of historical signals with outcomes
     */
    void calibrateFromHistory(const std::vector<SignalOutcome>& signal_history);

private:
    DecayModelType model_type_;
    DecayParameters params_;
    std::function<double(const TradingSignal&, uint64_t)> custom_decay_func_;
    std::map<ImbalanceType, double> type_specific_decay_rates_;
    std::map<ImbalanceType, uint64_t> type_specific_ttl_;
    std::vector<MarketEvent> recent_market_events_;
    
    /**
     * @brief Calculate exponential decay
     * @param signal The trading signal
     * @param current_time Current timestamp
     * @return Remaining signal strength
     */
    double exponentialDecay(const TradingSignal& signal, uint64_t current_time);
    
    /**
     * @brief Calculate linear decay
     * @param signal The trading signal
     * @param current_time Current timestamp
     * @return Remaining signal strength
     */
    double linearDecay(const TradingSignal& signal, uint64_t current_time);
    
    /**
     * @brief Calculate step-wise decay
     * @param signal The trading signal
     * @param current_time Current timestamp
     * @return Remaining signal strength
     */
    double stepDecay(const TradingSignal& signal, uint64_t current_time);
    
    /**
     * @brief Calculate conditional decay based on market events
     * @param signal The trading signal
     * @param current_time Current timestamp
     * @return Remaining signal strength
     */
    double conditionalDecay(const TradingSignal& signal, uint64_t current_time);
    
    /**
     * @brief Check if market events invalidate the signal
     * @param signal The trading signal
     * @return True if market events invalidate the signal
     */
    bool isInvalidatedByMarketEvents(const TradingSignal& signal);
    
    /**
     * @brief Get decay rate specific to the signal's imbalance type
     * @param signal The trading signal
     * @return Decay rate for this type of signal
     */
    double getTypeSpecificDecayRate(const TradingSignal& signal);
};

} // namespace strategy
} // namespace trading