#include "risk/posttrade/post_trade_monitor.hpp"
#include <algorithm>
#include <thread>
#include <chrono>

namespace trading {
namespace risk {

PostTradeMonitor::PostTradeMonitor(const RiskConfig& config)
    : m_config(config),
      m_mode(RiskManagerMode::NORMAL) {
}

bool PostTradeMonitor::initialize() {
    // Create sub-components with shared dependencies
    auto positionTracker = std::make_shared<PositionTracker>();
    auto dataInterface = std::make_shared<DataInterface>();
    auto executionInterface = std::make_shared<ExecutionInterface>();
    
    // Initialize circuit breaker
    m_circuitBreaker = std::make_unique<CircuitBreaker>(m_config);
    if (!m_circuitBreaker->initialize()) {
        return false;
    }
    
    // Initialize stop loss manager
    m_stopLossManager = std::make_unique<StopLossManager>(m_config, positionTracker, dataInterface, executionInterface);
    if (!m_stopLossManager->initialize()) {
        return false;
    }
    
    // Initialize drawdown monitor
    m_drawdownMonitor = std::make_unique<DrawdownMonitor>(m_config);
    if (!m_drawdownMonitor->initialize()) {
        return false;
    }
    
    // Initialize balance reconciliation
    m_balanceReconciliation = std::make_unique<BalanceReconciliation>(m_config, positionTracker, dataInterface, executionInterface);
    if (!m_balanceReconciliation->initialize()) {
        return false;
    }
    
    // Register internal callbacks
    
    // Circuit breaker state change callback
    m_circuitBreaker->registerStateChangeCallback(
        [this](CircuitBreakerState oldState, CircuitBreakerState newState, const std::string& reason) {
            // Create risk event
            RiskEvent event;
            event.type = RiskEventType::CIRCUIT_BREAKER;
            event.timestamp = std::chrono::system_clock::now();
            event.details = "Circuit breaker state changed: " + 
                          circuitBreakerStateToString(oldState) + " -> " + 
                          circuitBreakerStateToString(newState) + 
                          " (" + reason + ")";
            
            // Store circuit breaker specific data
            event.circuitBreakerEvent = CircuitBreakerEvent();
            event.circuitBreakerEvent->oldState = oldState;
            event.circuitBreakerEvent->newState = newState;
            event.circuitBreakerEvent->reason = reason;
            event.circuitBreakerEvent->triggerTime = std::chrono::system_clock::now();
            
            // Notify all registered callbacks
            notifyRiskEvent(event);
            
            // If circuit breaker triggered, update mode
            if (newState == CircuitBreakerState::TRIGGERED) {
                setMode(RiskManagerMode::EMERGENCY);
            } else if (newState == CircuitBreakerState::OFF && oldState != CircuitBreakerState::WARNING) {
                setMode(RiskManagerMode::NORMAL);
            }
        }
    );
    
    // Stop loss callback
    m_stopLossManager->registerStopLossCallback(
        [this](const StopLossEvent& stopLossEvent) {
            // Create risk event
            RiskEvent event;
            event.type = RiskEventType::STOP_LOSS;
            event.timestamp = std::chrono::system_clock::now();
            event.details = "Stop loss triggered for " + stopLossEvent.exchangeId + ":" + 
                          stopLossEvent.tradingPair + " at price " + std::to_string(stopLossEvent.triggerPrice);
            
            // Store stop loss specific data
            event.stopLossEvent = stopLossEvent;
            
            // Notify all registered callbacks
            notifyRiskEvent(event);
            
            // If critical stop loss, update mode
            if (stopLossEvent.lossPercentage >= m_config.stopLossConfig.criticalLossThreshold) {
                setMode(RiskManagerMode::HEIGHTENED_RISK);
            }
        }
    );
    
    // Drawdown callback
    m_drawdownMonitor->registerDrawdownCallback(
        [this](const DrawdownEvent& drawdownEvent) {
            // Create risk event
            RiskEvent event;
            event.type = RiskEventType::DRAWDOWN;
            event.timestamp = std::chrono::system_clock::now();
            event.details = "Drawdown threshold breach: " + 
                          (drawdownEvent.strategyId == "PORTFOLIO" ? "Portfolio" : "Strategy " + drawdownEvent.strategyId) + 
                          " drawdown of " + std::to_string(drawdownEvent.drawdownPercent * 100.0) + "% " +
                          (drawdownEvent.isCritical ? "(CRITICAL)" : "(WARNING)");
            
            // Store drawdown specific data
            event.drawdownEvent = drawdownEvent;
            
            // Notify all registered callbacks
            notifyRiskEvent(event);
            
            // If critical drawdown, update mode
            if (drawdownEvent.isCritical) {
                if (drawdownEvent.strategyId == "PORTFOLIO") {
                    setMode(RiskManagerMode::EMERGENCY);
                } else {
                    setMode(RiskManagerMode::HEIGHTENED_RISK);
                }
            }
        }
    );
    
    // Reconciliation callback
    m_balanceReconciliation->registerReconciliationCallback(
        [this](const PositionReconciliationEvent& reconciliationEvent) {
            // Create risk event
            RiskEvent event;
            event.type = RiskEventType::POSITION_RECONCILIATION;
            event.timestamp = std::chrono::system_clock::now();
            event.details = "Position reconciliation: " + reconciliationEvent.exchangeId + ":" +
                          reconciliationEvent.tradingPair + " discrepancy of " + 
                          std::to_string(reconciliationEvent.discrepancy) + 
                          (reconciliationEvent.autoCorrect ? " (auto-corrected)" : "");
            
            // Store reconciliation specific data
            event.reconciliationEvent = reconciliationEvent;
            
            // Notify all registered callbacks
            notifyRiskEvent(event);
            
            // If significant reconciliation issue, update mode
            if (std::abs(reconciliationEvent.discrepancyPercent) > m_config.balanceReconciliationConfig.criticalDiscrepancyPercent) {
                setMode(RiskManagerMode::HEIGHTENED_RISK);
            }
        }
    );
    
    // Start periodic tasks
    std::thread([this]() {
        while (true) {
            performScheduledTasks();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();
    
    return true;
}

void PostTradeMonitor::processExecutionReport(const ExecutionReport& executionReport) {
    // Check if trading is allowed
    if (!m_circuitBreaker->isTradingAllowed()) {
        RiskEvent event;
        event.type = RiskEventType::TRADING_BLOCKED;
        event.timestamp = std::chrono::system_clock::now();
        event.details = "Execution blocked by circuit breaker: " + 
                      executionReport.exchangeId + ":" + executionReport.tradingPair;
        notifyRiskEvent(event);
        return;
    }
    
    // Forward to stop loss manager
    m_stopLossManager->processExecutionReport(executionReport);
    
    // Check monitors after processing
    checkMonitors();
}

void PostTradeMonitor::updateMarketData(const MarketData& marketData) {
    // Forward to stop loss manager
    m_stopLossManager->updateMarketData(marketData);
    
    // Check monitors after update
    checkMonitors();
}

void PostTradeMonitor::updatePerformanceMetrics(const PerformanceMetrics& metrics) {
    // Forward to drawdown monitor
    auto drawdownEvents = m_drawdownMonitor->updatePerformanceMetrics(metrics);
    
    // Update circuit breaker metrics
    for (const auto& [metricName, value] : metrics.riskMetrics) {
        m_circuitBreaker->updateMetric(metricName, value);
    }
    
    // Check monitors after update
    checkMonitors();
}

void PostTradeMonitor::registerRiskEventCallback(const RiskEventCallback& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_eventCallbacks.push_back(callback);
}

void PostTradeMonitor::setMode(RiskManagerMode mode) {
    auto oldMode = m_mode.exchange(mode);
    
    // Skip if mode didn't change
    if (oldMode == mode) {
        return;
    }
    
    // Create risk event for mode change
    RiskEvent event;
    event.type = RiskEventType::RISK_MODE_CHANGE;
    event.timestamp = std::chrono::system_clock::now();
    event.details = "Risk manager mode changed: " + 
                  riskManagerModeToString(oldMode) + " -> " + 
                  riskManagerModeToString(mode);
    
    // Notify all registered callbacks
    notifyRiskEvent(event);
}

RiskManagerMode PostTradeMonitor::getMode() const {
    return m_mode.load();
}

CircuitBreakerState PostTradeMonitor::getCircuitBreakerState() const {
    return m_circuitBreaker->getState();
}

bool PostTradeMonitor::reconcilePositions(const std::optional<ExchangeId>& exchangeId) {
    if (exchangeId) {
        auto result = m_balanceReconciliation->reconcileExchangePositions(*exchangeId);
        return result.success;
    } else {
        auto result = m_balanceReconciliation->reconcileAllPositions();
        return result.success;
    }
}

bool PostTradeMonitor::updateConfiguration(const RiskConfig& config) {
    m_config = config;
    
    // Update sub-components
    bool success = true;
    success &= m_circuitBreaker->updateConfiguration(config.circuitBreakerConfig);
    success &= m_stopLossManager->updateConfiguration(config.stopLossConfig);
    success &= m_drawdownMonitor->updateConfiguration(config.drawdownConfig);
    success &= m_balanceReconciliation->updateConfiguration(config.balanceReconciliationConfig);
    
    return success;
}

StopLossManager& PostTradeMonitor::getStopLossManager() {
    return *m_stopLossManager;
}

DrawdownMonitor& PostTradeMonitor::getDrawdownMonitor() {
    return *m_drawdownMonitor;
}

CircuitBreaker& PostTradeMonitor::getCircuitBreaker() {
    return *m_circuitBreaker;
}

BalanceReconciliation& PostTradeMonitor::getBalanceReconciliation() {
    return *m_balanceReconciliation;
}

void PostTradeMonitor::notifyRiskEvent(const RiskEvent& event) {
    std::vector<RiskEventCallback> callbacksCopy;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callbacksCopy = m_eventCallbacks;
    }
    
    for (const auto& callback : callbacksCopy) {
        callback(event);
    }
}

bool PostTradeMonitor::checkMonitors() {
    // Check stop losses
    auto stopLossEvents = m_stopLossManager->checkStopLosses();
    
    // Check circuit breaker (already handled by callbacks, just get state)
    auto circuitBreakerState = m_circuitBreaker->getState();
    
    // Return true if no issues
    return stopLossEvents.empty() && circuitBreakerState != CircuitBreakerState::TRIGGERED;
}

void PostTradeMonitor::performScheduledTasks() {
    // Current time
    auto now = std::chrono::system_clock::now();
    
    // Check if in emergency mode for too long
    if (m_mode.load() == RiskManagerMode::EMERGENCY) {
        // TODO: Add emergency duration tracking and auto-recovery
    }
    
    // Additional periodic tasks can be added here
}

// Helper function to convert circuit breaker state to string
std::string PostTradeMonitor::circuitBreakerStateToString(CircuitBreakerState state) {
    switch (state) {
        case CircuitBreakerState::OFF: return "OFF";
        case CircuitBreakerState::WARNING: return "WARNING";
        case CircuitBreakerState::TRIGGERED: return "TRIGGERED";
        case CircuitBreakerState::COOLING: return "COOLING";
        default: return "UNKNOWN";
    }
}

// Helper function to convert risk manager mode to string
std::string PostTradeMonitor::riskManagerModeToString(RiskManagerMode mode) {
    switch (mode) {
        case RiskManagerMode::NORMAL: return "NORMAL";
        case RiskManagerMode::HEIGHTENED_RISK: return "HEIGHTENED_RISK";
        case RiskManagerMode::EMERGENCY: return "EMERGENCY";
        case RiskManagerMode::MAINTENANCE: return "MAINTENANCE";
        default: return "UNKNOWN";
    }
}

} // namespace risk
}