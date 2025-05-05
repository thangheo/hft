#include "risk/posttrade/circuit_breaker.hpp"
#include <algorithm>
#include <sstream>

namespace trading {
namespace risk {

CircuitBreaker::CircuitBreaker(const RiskConfig& config)
    : m_config(config),
      m_circuitBreakerConfig(config.circuitBreakerConfig),
      m_state(CircuitBreakerState::OFF),
      m_triggerCount(0) {
}

bool CircuitBreaker::initialize() {
    // Initialize state
    m_state = CircuitBreakerState::OFF;
    m_triggerTime = std::chrono::system_clock::time_point();
    m_coolingStartTime = std::chrono::system_clock::time_point();
    m_lastTriggerReason = "";
    m_triggerCount = 0;
    m_triggerCountResetTime = std::chrono::system_clock::now() + 
                             std::chrono::minutes(m_circuitBreakerConfig.triggerCountResetMinutes);
    
    return true;
}

CircuitBreakerState CircuitBreaker::evaluateMetrics(const std::map<std::string, double>& metrics) {
    // Current state
    auto currentState = m_state.load();
    
    // If already triggered, only process auto-reset
    if (currentState == CircuitBreakerState::TRIGGERED) {
        if (shouldAutoReset()) {
            setState(CircuitBreakerState::COOLING, "Auto cooling after mandatory halt period");
        }
        return m_state.load();
    }
    
    // If in cooling state, check for gradual trading resumption
    if (currentState == CircuitBreakerState::COOLING) {
        auto now = std::chrono::system_clock::now();
        auto coolingElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_coolingStartTime).count();
        
        if (coolingElapsed >= m_circuitBreakerConfig.coolingPeriodSeconds) {
            setState(CircuitBreakerState::OFF, "Cooling period completed");
        }
        
        return m_state.load();
    }
    
    // Check for critical thresholds
    for (const auto& [metricName, value] : metrics) {
        // Skip metrics that aren't configured
        if (m_circuitBreakerConfig.metricThresholds.find(metricName) == 
            m_circuitBreakerConfig.metricThresholds.end()) {
            continue;
        }
        
        double threshold = 0.0;
        if (isMetricCritical(metricName, value, threshold)) {
            // Critical threshold breached, trigger circuit breaker
            m_triggerCount++;
            checkTriggerCount();
            
            std::ostringstream reason;
            reason << "Critical threshold breached for metric '" << metricName
                   << "': " << value << " (threshold: " << threshold << ")";
            
            setState(CircuitBreakerState::TRIGGERED, reason.str());
            
            // Create circuit breaker event
            auto event = createCircuitBreakerEvent(CircuitBreakerState::OFF, CircuitBreakerState::TRIGGERED,
                                                metricName, threshold, value);
            
            // Store in history
            {
                std::lock_guard<std::mutex> lock(m_historyMutex);
                m_triggerHistory.push_back(event);
                
                // Limit history size
                if (m_triggerHistory.size() > m_circuitBreakerConfig.maxHistorySize) {
                    m_triggerHistory.erase(m_triggerHistory.begin());
                }
            }
            
            return m_state.load();
        }
        
        // Check for warning thresholds
        if (currentState == CircuitBreakerState::OFF) {
            if (isMetricWarning(metricName, value, threshold)) {
                // Warning threshold breached, set to warning state
                std::ostringstream reason;
                reason << "Warning threshold breached for metric '" << metricName
                       << "': " << value << " (threshold: " << threshold << ")";
                
                setState(CircuitBreakerState::WARNING, reason.str());
                
                // Create circuit breaker event
                auto event = createCircuitBreakerEvent(CircuitBreakerState::OFF, CircuitBreakerState::WARNING,
                                                    metricName, threshold, value);
                
                // Store in history
                {
                    std::lock_guard<std::mutex> lock(m_historyMutex);
                    m_triggerHistory.push_back(event);
                    
                    // Limit history size
                    if (m_triggerHistory.size() > m_circuitBreakerConfig.maxHistorySize) {
                        m_triggerHistory.erase(m_triggerHistory.begin());
                    }
                }
            }
        }
    }
    
    // If warning state and all metrics are now below warning threshold, return to OFF
    if (currentState == CircuitBreakerState::WARNING) {
        bool allBelowWarning = true;
        
        for (const auto& [metricName, value] : metrics) {
            // Skip metrics that aren't configured
            if (m_circuitBreakerConfig.metricThresholds.find(metricName) == 
                m_circuitBreakerConfig.metricThresholds.end()) {
                continue;
            }
            
            double threshold = 0.0;
            if (isMetricWarning(metricName, value, threshold)) {
                allBelowWarning = false;
                break;
            }
        }
        
        if (allBelowWarning) {
            setState(CircuitBreakerState::OFF, "All metrics below warning thresholds");
        }
    }
    
    return m_state.load();
}

CircuitBreakerState CircuitBreaker::updateMetric(const std::string& metricName, double value) {
    // Update metric value
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_currentMetrics[metricName] = value;
    }
    
    // Evaluate all metrics
    std::map<std::string, double> metricsCopy;
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        metricsCopy = m_currentMetrics;
    }
    
    return evaluateMetrics(metricsCopy);
}

CircuitBreakerState CircuitBreaker::getState() const {
    return m_state.load();
}

bool CircuitBreaker::manualTrigger(const std::string& reason) {
    // Only allow if not already triggered
    auto currentState = m_state.load();
    if (currentState == CircuitBreakerState::TRIGGERED) {
        return false;
    }
    
    // Trigger the circuit breaker
    m_triggerCount++;
    checkTriggerCount();
    
    std::string fullReason = "Manual trigger: " + reason;
    setState(CircuitBreakerState::TRIGGERED, fullReason);
    
    // Create circuit breaker event
    auto event = createCircuitBreakerEvent(currentState, CircuitBreakerState::TRIGGERED,
                                        "MANUAL", 0.0, 0.0);
    
    // Store in history
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_triggerHistory.push_back(event);
        
        // Limit history size
        if (m_triggerHistory.size() > m_circuitBreakerConfig.maxHistorySize) {
            m_triggerHistory.erase(m_triggerHistory.begin());
        }
    }
    
    return true;
}

bool CircuitBreaker::reset() {
    auto currentState = m_state.load();
    
    // Can't reset directly from TRIGGERED state before mandatory halt period
    if (currentState == CircuitBreakerState::TRIGGERED) {
        auto now = std::chrono::system_clock::now();
        auto haltElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_triggerTime).count();
        
        if (haltElapsed < m_circuitBreakerConfig.mandatoryHaltPeriodSeconds) {
            return false;
        }
        
        // Move to cooling state first
        setState(CircuitBreakerState::COOLING, "Manual reset from TRIGGERED to COOLING");
    }
    
    // Now reset to OFF state
    setState(CircuitBreakerState::OFF, "Manual reset");
    return true;
}

bool CircuitBreaker::isTradingAllowed() const {
    auto state = m_state.load();
    
    // Trading is allowed in OFF state
    if (state == CircuitBreakerState::OFF) {
        return true;
    }
    
    // Trading is allowed in WARNING state
    if (state == CircuitBreakerState::WARNING) {
        return true;
    }
    
    // Trading is not allowed in TRIGGERED state
    if (state == CircuitBreakerState::TRIGGERED) {
        return false;
    }
    
    // In COOLING state, trading is gradually resumed
    if (state == CircuitBreakerState::COOLING) {
        auto now = std::chrono::system_clock::now();
        auto coolingElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_coolingStartTime).count();
        
        // Calculate what percentage of the cooling period has elapsed
        double coolingProgress = static_cast<double>(coolingElapsed) / m_circuitBreakerConfig.coolingPeriodSeconds;
        
        // Gradual trading resumption based on configuration
        if (m_circuitBreakerConfig.gradualTradingResumption) {
            // Allow trading with probability proportional to cooling progress
            return ((double)rand() / RAND_MAX) < coolingProgress;
        } else {
            // Binary trading resumption after cooling period
            return false;
        }
    }
    
    // Default to not allowing trading in unknown states
    return false;
}

void CircuitBreaker::registerStateChangeCallback(
    const std::function<void(CircuitBreakerState, CircuitBreakerState, const std::string&)>& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_stateChangeCallbacks.push_back(callback);
}

bool CircuitBreaker::updateConfiguration(const CircuitBreakerConfig& circuitBreakerConfig) {
    // Validate configuration
    if (circuitBreakerConfig.mandatoryHaltPeriodSeconds <= 0 ||
        circuitBreakerConfig.coolingPeriodSeconds <= 0 ||
        circuitBreakerConfig.maxTriggersBeforeExtendedHalt <= 0 ||
        circuitBreakerConfig.triggerCountResetMinutes <= 0) {
        return false;
    }
    
    m_circuitBreakerConfig = circuitBreakerConfig;
    return true;
}

CircuitBreakerConfig CircuitBreaker::getConfiguration() const {
    return m_circuitBreakerConfig;
}

int CircuitBreaker::getSecondsUntilReset() const {
    auto state = m_state.load();
    auto now = std::chrono::system_clock::now();
    
    if (state == CircuitBreakerState::TRIGGERED) {
        auto haltElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_triggerTime).count();
        int remainingSeconds = m_circuitBreakerConfig.mandatoryHaltPeriodSeconds - haltElapsed;
        return std::max(0, remainingSeconds);
    } else if (state == CircuitBreakerState::COOLING) {
        auto coolingElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_coolingStartTime).count();
        int remainingSeconds = m_circuitBreakerConfig.coolingPeriodSeconds - coolingElapsed;
        return std::max(0, remainingSeconds);
    }
    
    return 0;
}

std::vector<CircuitBreakerEvent> CircuitBreaker::getTriggerHistory() const {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    return m_triggerHistory;
}

bool CircuitBreaker::isMetricCritical(const std::string& metricName, double value, double& threshold) {
    auto it = m_circuitBreakerConfig.metricThresholds.find(metricName);
    if (it == m_circuitBreakerConfig.metricThresholds.end()) {
        return false;
    }
    
    const auto& thresholds = it->second;
    threshold = thresholds.criticalThreshold;
    
    // Check if value exceeds critical threshold
    if (thresholds.isHigherBetter) {
        return value < thresholds.criticalThreshold;
    } else {
        return value > thresholds.criticalThreshold;
    }
}

bool CircuitBreaker::isMetricWarning(const std::string& metricName, double value, double& threshold) {
    auto it = m_circuitBreakerConfig.metricThresholds.find(metricName);
    if (it == m_circuitBreakerConfig.metricThresholds.end()) {
        return false;
    }
    
    const auto& thresholds = it->second;
    threshold = thresholds.warningThreshold;
    
    // Check if value exceeds warning threshold
    if (thresholds.isHigherBetter) {
        return value < thresholds.warningThreshold;
    } else {
        return value > thresholds.warningThreshold;
    }
}

void CircuitBreaker::setState(CircuitBreakerState newState, const std::string& reason) {
    auto oldState = m_state.exchange(newState);
    
    // Skip if state didn't change
    if (oldState == newState) {
        return;
    }
    
    // Update state-specific data
    if (newState == CircuitBreakerState::TRIGGERED) {
        m_triggerTime = std::chrono::system_clock::now();
        m_lastTriggerReason = reason;
    } else if (newState == CircuitBreakerState::COOLING) {
        m_coolingStartTime = std::chrono::system_clock::now();
    }
    
    // Notify callbacks
    std::vector<std::function<void(CircuitBreakerState, CircuitBreakerState, const std::string&)>> callbacksCopy;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callbacksCopy = m_stateChangeCallbacks;
    }
    
    for (const auto& callback : callbacksCopy) {
        callback(oldState, newState, reason);
    }
}

bool CircuitBreaker::shouldAutoReset() const {
    auto now = std::chrono::system_clock::now();
    auto haltElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_triggerTime).count();
    
    // Calculate required halt period based on trigger count
    int requiredHaltPeriod = m_circuitBreakerConfig.mandatoryHaltPeriodSeconds;
    
    // Extend halt period if trigger count exceeds threshold
    if (m_triggerCount > m_circuitBreakerConfig.maxTriggersBeforeExtendedHalt) {
        requiredHaltPeriod *= m_circuitBreakerConfig.extendedHaltMultiplier;
    }
    
    return haltElapsed >= requiredHaltPeriod;
}

void CircuitBreaker::checkTriggerCount() {
    auto now = std::chrono::system_clock::now();
    
    // Reset trigger count if reset time has passed
    if (now > m_triggerCountResetTime) {
        m_triggerCount = 1; // Set to 1 because this is a new trigger
        m_triggerCountResetTime = now + std::chrono::minutes(m_circuitBreakerConfig.triggerCountResetMinutes);
    }
}

CircuitBreakerEvent CircuitBreaker::createCircuitBreakerEvent(CircuitBreakerState oldState, CircuitBreakerState newState, 
                                                           const std::string& metricName, double threshold, double value) {
    CircuitBreakerEvent event;
    event.oldState = oldState;
    event.newState = newState;
    event.triggerTime = std::chrono::system_clock::now();
    event.reason = m_lastTriggerReason;
    event.triggerMetric = metricName;
    event.metricValue = value;
    event.metricThreshold = threshold;
    event.triggerCount = m_triggerCount;
    return event;
}

} // namespace risk
}