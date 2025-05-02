/**
 * Interface for tracking performance against risk thresholds
 *
 * File: drawdown_monitor.hpp
 * Created: 2025-04-29
 */
#pragma once

#include "../risk_types.hpp"
#include "../risk_events.hpp"
#include "../risk_config.hpp"

#include <map>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <deque>
#include <memory>
#include <functional>

namespace risk {

/**
 * Structure to hold high water mark information
 */
struct HighWaterMark {
    double value;
    Timestamp timestamp;
};

/**
 * Structure to hold drawdown information
 */
struct DrawdownInfo {
    double currentValue;
    HighWaterMark highWaterMark;
    double drawdownPercent;
    bool warningThresholdBreached;
    bool criticalThresholdBreached;
    Timestamp lastUpdated;
};

/**
 * DrawdownMonitor tracks performance against risk thresholds.
 * It monitors drawdown at multiple levels and triggers risk responses when thresholds are breached.
 */
class DrawdownMonitor {
public:
    /**
     * Constructor
     * @param config Risk configuration containing drawdown monitoring settings
     */
    explicit DrawdownMonitor(const RiskConfig& config);
    
    /**
     * Initialize the drawdown monitor
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Update performance metrics
     * @param metrics New performance metrics
     * @return Vector of drawdown events if thresholds are breached
     */
    std::vector<DrawdownEvent> updatePerformanceMetrics(const PerformanceMetrics& metrics);
    
    /**
     * Update a specific strategy's performance value
     * @param strategyId The strategy identifier
     * @param value The current performance value
     * @return Optional drawdown event if threshold is breached
     */
    std::optional<DrawdownEvent> updateStrategyValue(const StrategyId& strategyId, double value);
    
    /**
     * Update the overall portfolio value
     * @param value The current portfolio value
     * @return Optional drawdown event if threshold is breached
     */
    std::optional<DrawdownEvent> updatePortfolioValue(double value);
    
    /**
     * Get the current drawdown information for a strategy
     * @param strategyId The strategy identifier
     * @return The drawdown information
     */
    DrawdownInfo getStrategyDrawdown(const StrategyId& strategyId) const;
    
    /**
     * Get the current portfolio drawdown information
     * @return The drawdown information
     */
    DrawdownInfo getPortfolioDrawdown() const;
    
    /**
     * Update drawdown configuration
     * @param drawdownConfig New drawdown configuration
     * @return true if update was successful, false otherwise
     */
    bool updateConfiguration(const DrawdownConfig& drawdownConfig);
    
    /**
     * Get the current drawdown configuration
     * @return The current configuration
     */
    DrawdownConfig getConfiguration() const;
    
    /**
     * Register a callback for drawdown events
     * @param callback The function to call when drawdown events occur
     */
    void registerDrawdownCallback(const std::function<void(const DrawdownEvent&)>& callback);
    
    /**
     * Reset high water mark for a strategy
     * @param strategyId The strategy identifier
     * @param value Optional new high water mark value (uses current value if not provided)
     * @return true if reset was successful, false otherwise
     */
    bool resetHighWaterMark(const StrategyId& strategyId, std::optional<double> value = std::nullopt);
    
    /**
     * Reset portfolio high water mark
     * @param value Optional new high water mark value (uses current value if not provided)
     * @return true if reset was successful, false otherwise
     */
    bool resetPortfolioHighWaterMark(std::optional<double> value = std::nullopt);

private:
    RiskConfig m_config;
    DrawdownConfig m_drawdownConfig;
    
    // Drawdown tracking
    mutable std::mutex m_strategyDrawdownMutex;
    std::unordered_map<std::string, DrawdownInfo> m_strategyDrawdowns;
    
    mutable std::mutex m_portfolioDrawdownMutex;
    DrawdownInfo m_portfolioDrawdown;
    
    // Historical values for lookback period
    mutable std::mutex m_historyMutex;
    std::unordered_map<std::string, std::deque<std::pair<Timestamp, double>>> m_strategyValueHistory;
    std::deque<std::pair<Timestamp, double>> m_portfolioValueHistory;
    
    // Callbacks
    mutable std::mutex m_callbackMutex;
    std::vector<std::function<void(const DrawdownEvent&)>> m_drawdownCallbacks;
    
    // Helper methods
    double calculateDrawdownPercent(double currentValue, double highWaterMark) const;
    DrawdownEvent createDrawdownEvent(const StrategyId& strategyId, double drawdownPercent, 
                                    double highWaterMark, double currentValue, Timestamp hwmTime, 
                                    bool thresholdBreached);
    void updateHighWaterMark(DrawdownInfo& drawdownInfo, double currentValue);
    void pruneHistory();
};

} // namespace risk
