/**
 * Interface for the unified post-trade monitoring system
 *
 * File: post_trade_monitor.hpp
 * Created: 2025-04-29
 */
#pragma once

#include "../risk_types.hpp"
#include "../risk_events.hpp"
#include "../risk_config.hpp"
#include "circuit_breaker.hpp"
#include "stop_loss_manager.hpp"
#include "drawdown_monitor.hpp"
#include "balance_reconciliation.hpp"

#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace trading    {   
namespace risk {

/**
 * PostTradeMonitor is the central component for post-trade risk monitoring.
 * It coordinates all post-trade risk controls.
 */
class PostTradeMonitor {
public:
    /**
     * Constructor
     * @param config Risk configuration containing post-trade monitoring settings
     */
    explicit PostTradeMonitor(const RiskConfig& config);
    
    /**
     * Initialize the monitor and its components
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Process an execution report
     * @param executionReport The execution report to process
     */
    void processExecutionReport(const ExecutionReport& executionReport);
    
    /**
     * Update market data used for monitoring
     * @param marketData New market data
     */
    void updateMarketData(const MarketData& marketData);
    
    /**
     * Update performance metrics
     * @param metrics New performance metrics
     */
    void updatePerformanceMetrics(const PerformanceMetrics& metrics);
    
    /**
     * Register a callback for risk events
     * @param callback The function to call when risk events occur
     */
    void registerRiskEventCallback(const RiskEventCallback& callback);
    
    /**
     * Set the current risk manager mode
     * @param mode The new mode
     */
    void setMode(RiskManagerMode mode);
    
    /**
     * Get the current risk manager mode
     * @return The current mode
     */
    RiskManagerMode getMode() const;
    
    /**
     * Get the circuit breaker status
     * @return The current circuit breaker state
     */
    CircuitBreakerState getCircuitBreakerState() const;
    
    /**
     * Force a position reconciliation
     * @param exchangeId Optional exchange ID to limit reconciliation
     * @return true if reconciliation was successful, false otherwise
     */
    bool reconcilePositions(const std::optional<ExchangeId>& exchangeId = std::nullopt);
    
    /**
     * Update post-trade monitoring configuration
     * @param config New configuration
     * @return true if update was successful, false otherwise
     */
    bool updateConfiguration(const RiskConfig& config);
    
    /**
     * Get stop loss manager
     * @return Reference to the stop loss manager
     */
    StopLossManager& getStopLossManager();
    
    /**
     * Get drawdown monitor
     * @return Reference to the drawdown monitor
     */
    DrawdownMonitor& getDrawdownMonitor();
    
    /**
     * Get circuit breaker
     * @return Reference to the circuit breaker
     */
    CircuitBreaker& getCircuitBreaker();
    
    /**
     * Get balance reconciliation
     * @return Reference to the balance reconciliation
     */
    BalanceReconciliation& getBalanceReconciliation();

private:
    // Sub-components
    std::unique_ptr<CircuitBreaker> m_circuitBreaker;
    std::unique_ptr<StopLossManager> m_stopLossManager;
    std::unique_ptr<DrawdownMonitor> m_drawdownMonitor;
    std::unique_ptr<BalanceReconciliation> m_balanceReconciliation;
    
    // Configuration and state
    RiskConfig m_config;
    std::atomic<RiskManagerMode> m_mode;
    
    // Risk event callbacks
    std::vector<RiskEventCallback> m_eventCallbacks;
    mutable std::mutex m_callbackMutex;
    
    // Helper methods
    void notifyRiskEvent(const RiskEvent& event);
    bool checkMonitors();
    void performScheduledTasks();
};

} // namespace risk
}