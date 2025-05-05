/**
 * Interface for trading halt mechanism during extreme market conditions
 *
 * File: circuit_breaker.hpp
 * Created: 2025-04-29
 */
#pragma once

#include "../risk_types.hpp"
#include "../risk_events.hpp"
#include "../risk_config.hpp"

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <functional>
#include <mutex>
#include <chrono>
#include <atomic>

namespace trading {
namespace risk {

/**
 * CircuitBreaker halts trading during extreme market conditions.
 * It monitors trading performance metrics and can automatically stop trading when thresholds are breached.
 */
class CircuitBreaker {
public:
    /**
     * Constructor
     * @param config Risk configuration containing circuit breaker settings
     */
    explicit CircuitBreaker(const RiskConfig& config);
    
    /**
     * Initialize the circuit breaker
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Evaluate metrics to determine if the circuit breaker should trigger
     * @param metrics The metrics to evaluate
     * @return The new circuit breaker state
     */
    CircuitBreakerState evaluateMetrics(const std::map<std::string, double>& metrics);
    
    /**
     * Update a specific metric value
     * @param metricName The name of the metric
     * @param value The new value of the metric
     * @return The new circuit breaker state
     */
    CircuitBreakerState updateMetric(const std::string& metricName, double value);
    
    /**
     * Get the current circuit breaker state
     * @return The current state
     */
    CircuitBreakerState getState() const;
    
    /**
     * Manually trigger the circuit breaker
     * @param reason The reason for manual triggering
     * @return true if successful, false otherwise
     */
    bool manualTrigger(const std::string& reason);
    
    /**
     * Reset the circuit breaker to normal state
     * @return true if successful, false otherwise
     */
    bool reset();
    
    /**
     * Check if trading is allowed
     * @return true if trading is allowed, false otherwise
     */
    bool isTradingAllowed() const;
    
    /**
     * Register a callback for circuit breaker state changes
     * @param callback The function to call when state changes
     */
    void registerStateChangeCallback(const std::function<void(CircuitBreakerState, CircuitBreakerState, const std::string&)>& callback);
    
    /**
     * Update circuit breaker configuration
     * @param circuitBreakerConfig New circuit breaker configuration
     * @return true if update was successful, false otherwise
     */
    bool updateConfiguration(const CircuitBreakerConfig& circuitBreakerConfig);
    
    /**
     * Get the current circuit breaker configuration
     * @return The current configuration
     */
    CircuitBreakerConfig getConfiguration() const;
    
    /**
     * Get estimated time until automatic reset
     * @return The estimated time in seconds, or 0 if not in cooling state
     */
    int getSecondsUntilReset() const;
    
    /**
     * Get the trigger history
     * @return Vector of trigger events
     */
    std::vector<CircuitBreakerEvent> getTriggerHistory() const;

private:
    // Configuration
    RiskConfig m_config;
    CircuitBreakerConfig m_circuitBreakerConfig;
    
    // State
    std::atomic<CircuitBreakerState> m_state;
    std::chrono::system_clock::time_point m_triggerTime;
    std::chrono::system_clock::time_point m_coolingStartTime;
    std::string m_lastTriggerReason;
    int m_triggerCount;
    std::chrono::system_clock::time_point m_triggerCountResetTime;
    
    // Metrics
    mutable std::mutex m_metricsMutex;
    std::unordered_map<std::string, double> m_currentMetrics;
    
    // History
    mutable std::mutex m_historyMutex;
    std::vector<CircuitBreakerEvent> m_triggerHistory;
    
    // Callbacks
    mutable std::mutex m_callbackMutex;
    std::vector<std::function<void(CircuitBreakerState, CircuitBreakerState, const std::string&)>> m_stateChangeCallbacks;
    
    // Helper methods
    bool isMetricCritical(const std::string& metricName, double value, double& threshold);
    bool isMetricWarning(const std::string& metricName, double value, double& threshold);
    void setState(CircuitBreakerState newState, const std::string& reason);
    bool shouldAutoReset() const;
    void checkTriggerCount();
    CircuitBreakerEvent createCircuitBreakerEvent(CircuitBreakerState oldState, CircuitBreakerState newState, 
                                                const std::string& metricName, double threshold, double value);
};

} // namespace risk
}