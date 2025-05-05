#include "risk/posttrade/drawdown_monitor.hpp"
#include <algorithm>
#include <cmath>

namespace trading {
namespace risk {

DrawdownMonitor::DrawdownMonitor(const RiskConfig& config)
    : m_config(config),
      m_drawdownConfig(config.drawdownConfig) {
}

bool DrawdownMonitor::initialize() {
    // Initialize portfolio drawdown info
    m_portfolioDrawdown.currentValue = 0.0;
    m_portfolioDrawdown.highWaterMark.value = 0.0;
    m_portfolioDrawdown.highWaterMark.timestamp = std::chrono::system_clock::now();
    m_portfolioDrawdown.drawdownPercent = 0.0;
    m_portfolioDrawdown.warningThresholdBreached = false;
    m_portfolioDrawdown.criticalThresholdBreached = false;
    m_portfolioDrawdown.lastUpdated = std::chrono::system_clock::now();
    
    // Initialize history containers
    m_portfolioValueHistory.clear();
    m_strategyValueHistory.clear();
    
    return true;
}

std::vector<DrawdownEvent> DrawdownMonitor::updatePerformanceMetrics(const PerformanceMetrics& metrics) {
    std::vector<DrawdownEvent> events;
    
    // Update portfolio value
    auto portfolioEvent = updatePortfolioValue(metrics.portfolioValue);
    if (portfolioEvent) {
        events.push_back(*portfolioEvent);
    }
    
    // Update strategy values
    for (const auto& [strategyId, strategyMetrics] : metrics.strategyMetrics) {
        auto strategyEvent = updateStrategyValue(strategyId, strategyMetrics.value);
        if (strategyEvent) {
            events.push_back(*strategyEvent);
        }
    }
    
    // Prune historical data if needed
    pruneHistory();
    
    return events;
}

std::optional<DrawdownEvent> DrawdownMonitor::updateStrategyValue(const StrategyId& strategyId, double value) {
    // Current timestamp
    auto now = std::chrono::system_clock::now();
    
    // Get or create drawdown info for this strategy
    DrawdownInfo drawdownInfo;
    {
        std::lock_guard<std::mutex> lock(m_strategyDrawdownMutex);
        auto it = m_strategyDrawdowns.find(strategyId);
        if (it != m_strategyDrawdowns.end()) {
            drawdownInfo = it->second;
        } else {
            // Initialize new drawdown info
            drawdownInfo.currentValue = value;
            drawdownInfo.highWaterMark.value = value;
            drawdownInfo.highWaterMark.timestamp = now;
            drawdownInfo.drawdownPercent = 0.0;
            drawdownInfo.warningThresholdBreached = false;
            drawdownInfo.criticalThresholdBreached = false;
        }
    }
    
    // Store value in history
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_strategyValueHistory[strategyId].push_back({now, value});
    }
    
    // Update current value
    drawdownInfo.currentValue = value;
    drawdownInfo.lastUpdated = now;
    
    // Update high water mark if needed
    if (value > drawdownInfo.highWaterMark.value) {
        updateHighWaterMark(drawdownInfo, value);
    }
    
    // Calculate drawdown percentage
    double oldDrawdownPercent = drawdownInfo.drawdownPercent;
    drawdownInfo.drawdownPercent = calculateDrawdownPercent(value, drawdownInfo.highWaterMark.value);
    
    // Check threshold breaches
    bool wasWarningBreached = drawdownInfo.warningThresholdBreached;
    bool wasCriticalBreached = drawdownInfo.criticalThresholdBreached;
    
    // Check if warning threshold is breached
    if (drawdownInfo.drawdownPercent >= m_drawdownConfig.strategyWarningThreshold) {
        drawdownInfo.warningThresholdBreached = true;
    } else {
        drawdownInfo.warningThresholdBreached = false;
    }
    
    // Check if critical threshold is breached
    if (drawdownInfo.drawdownPercent >= m_drawdownConfig.strategyCriticalThreshold) {
        drawdownInfo.criticalThresholdBreached = true;
    } else {
        drawdownInfo.criticalThresholdBreached = false;
    }
    
    // Update drawdown info in map
    {
        std::lock_guard<std::mutex> lock(m_strategyDrawdownMutex);
        m_strategyDrawdowns[strategyId] = drawdownInfo;
    }
    
    // Create event if threshold breach has changed
    if (!wasCriticalBreached && drawdownInfo.criticalThresholdBreached) {
        // Critical breach just occurred
        auto event = createDrawdownEvent(strategyId, drawdownInfo.drawdownPercent,
                                      drawdownInfo.highWaterMark.value, value,
                                      drawdownInfo.highWaterMark.timestamp, true);
        
        // Notify callbacks
        for (const auto& callback : m_drawdownCallbacks) {
            callback(event);
        }
        
        return event;
    } else if (!wasWarningBreached && drawdownInfo.warningThresholdBreached) {
        // Warning breach just occurred
        auto event = createDrawdownEvent(strategyId, drawdownInfo.drawdownPercent,
                                      drawdownInfo.highWaterMark.value, value,
                                      drawdownInfo.highWaterMark.timestamp, false);
        
        // Notify callbacks
        for (const auto& callback : m_drawdownCallbacks) {
            callback(event);
        }
        
        return event;
    }
    
    return std::nullopt;
}

std::optional<DrawdownEvent> DrawdownMonitor::updatePortfolioValue(double value) {
    // Current timestamp
    auto now = std::chrono::system_clock::now();
    
    // Get current portfolio drawdown info
    DrawdownInfo drawdownInfo;
    {
        std::lock_guard<std::mutex> lock(m_portfolioDrawdownMutex);
        drawdownInfo = m_portfolioDrawdown;
    }
    
    // Store value in history
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_portfolioValueHistory.push_back({now, value});
    }
    
    // Update current value
    drawdownInfo.currentValue = value;
    drawdownInfo.lastUpdated = now;
    
    // Initialize high water mark if this is the first value
    if (drawdownInfo.highWaterMark.value == 0.0) {
        updateHighWaterMark(drawdownInfo, value);
    }
    // Update high water mark if needed
    else if (value > drawdownInfo.highWaterMark.value) {
        updateHighWaterMark(drawdownInfo, value);
    }
    
    // Calculate drawdown percentage
    double oldDrawdownPercent = drawdownInfo.drawdownPercent;
    drawdownInfo.drawdownPercent = calculateDrawdownPercent(value, drawdownInfo.highWaterMark.value);
    
    // Check threshold breaches
    bool wasWarningBreached = drawdownInfo.warningThresholdBreached;
    bool wasCriticalBreached = drawdownInfo.criticalThresholdBreached;
    
    // Check if warning threshold is breached
    if (drawdownInfo.drawdownPercent >= m_drawdownConfig.portfolioWarningThreshold) {
        drawdownInfo.warningThresholdBreached = true;
    } else {
        drawdownInfo.warningThresholdBreached = false;
    }
    
    // Check if critical threshold is breached
    if (drawdownInfo.drawdownPercent >= m_drawdownConfig.portfolioCriticalThreshold) {
        drawdownInfo.criticalThresholdBreached = true;
    } else {
        drawdownInfo.criticalThresholdBreached = false;
    }
    
    // Update portfolio drawdown info
    {
        std::lock_guard<std::mutex> lock(m_portfolioDrawdownMutex);
        m_portfolioDrawdown = drawdownInfo;
    }
    
    // Create event if threshold breach has changed
    if (!wasCriticalBreached && drawdownInfo.criticalThresholdBreached) {
        // Critical breach just occurred
        auto event = createDrawdownEvent("PORTFOLIO", drawdownInfo.drawdownPercent,
                                      drawdownInfo.highWaterMark.value, value,
                                      drawdownInfo.highWaterMark.timestamp, true);
        
        // Notify callbacks
        for (const auto& callback : m_drawdownCallbacks) {
            callback(event);
        }
        
        return event;
    } else if (!wasWarningBreached && drawdownInfo.warningThresholdBreached) {
        // Warning breach just occurred
        auto event = createDrawdownEvent("PORTFOLIO", drawdownInfo.drawdownPercent,
                                      drawdownInfo.highWaterMark.value, value,
                                      drawdownInfo.highWaterMark.timestamp, false);
        
        // Notify callbacks
        for (const auto& callback : m_drawdownCallbacks) {
            callback(event);
        }
        
        return event;
    }
    
    return std::nullopt;
}

DrawdownInfo DrawdownMonitor::getStrategyDrawdown(const StrategyId& strategyId) const {
    std::lock_guard<std::mutex> lock(m_strategyDrawdownMutex);
    auto it = m_strategyDrawdowns.find(strategyId);
    if (it != m_strategyDrawdowns.end()) {
        return it->second;
    }
    
    // Return default drawdown info if not found
    DrawdownInfo defaultInfo;
    defaultInfo.currentValue = 0.0;
    defaultInfo.highWaterMark.value = 0.0;
    defaultInfo.highWaterMark.timestamp = std::chrono::system_clock::now();
    defaultInfo.drawdownPercent = 0.0;
    defaultInfo.warningThresholdBreached = false;
    defaultInfo.criticalThresholdBreached = false;
    defaultInfo.lastUpdated = std::chrono::system_clock::now();
    return defaultInfo;
}

DrawdownInfo DrawdownMonitor::getPortfolioDrawdown() const {
    std::lock_guard<std::mutex> lock(m_portfolioDrawdownMutex);
    return m_portfolioDrawdown;
}

bool DrawdownMonitor::updateConfiguration(const DrawdownConfig& drawdownConfig) {
    // Validate thresholds
    if (drawdownConfig.portfolioWarningThreshold <= 0 ||
        drawdownConfig.portfolioCriticalThreshold <= 0 ||
        drawdownConfig.strategyWarningThreshold <= 0 ||
        drawdownConfig.strategyCriticalThreshold <= 0 ||
        drawdownConfig.portfolioWarningThreshold >= drawdownConfig.portfolioCriticalThreshold ||
        drawdownConfig.strategyWarningThreshold >= drawdownConfig.strategyCriticalThreshold) {
        return false;
    }
    
    m_drawdownConfig = drawdownConfig;
    return true;
}

DrawdownConfig DrawdownMonitor::getConfiguration() const {
    return m_drawdownConfig;
}

void DrawdownMonitor::registerDrawdownCallback(const std::function<void(const DrawdownEvent&)>& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_drawdownCallbacks.push_back(callback);
}

bool DrawdownMonitor::resetHighWaterMark(const StrategyId& strategyId, std::optional<double> value) {
    std::lock_guard<std::mutex> lock(m_strategyDrawdownMutex);
    
    auto it = m_strategyDrawdowns.find(strategyId);
    if (it == m_strategyDrawdowns.end()) {
        return false;
    }
    
    // Update high water mark
    it->second.highWaterMark.timestamp = std::chrono::system_clock::now();
    
    if (value) {
        it->second.highWaterMark.value = *value;
    } else {
        it->second.highWaterMark.value = it->second.currentValue;
    }
    
    // Recalculate drawdown
    it->second.drawdownPercent = calculateDrawdownPercent(it->second.currentValue, it->second.highWaterMark.value);
    
    // Reset threshold breaches
    it->second.warningThresholdBreached = false;
    it->second.criticalThresholdBreached = false;
    
    return true;
}

bool DrawdownMonitor::resetPortfolioHighWaterMark(std::optional<double> value) {
    std::lock_guard<std::mutex> lock(m_portfolioDrawdownMutex);
    
    // Update high water mark
    m_portfolioDrawdown.highWaterMark.timestamp = std::chrono::system_clock::now();
    
    if (value) {
        m_portfolioDrawdown.highWaterMark.value = *value;
    } else {
        m_portfolioDrawdown.highWaterMark.value = m_portfolioDrawdown.currentValue;
    }
    
    // Recalculate drawdown
    m_portfolioDrawdown.drawdownPercent = calculateDrawdownPercent(m_portfolioDrawdown.currentValue, 
                                                                 m_portfolioDrawdown.highWaterMark.value);
    
    // Reset threshold breaches
    m_portfolioDrawdown.warningThresholdBreached = false;
    m_portfolioDrawdown.criticalThresholdBreached = false;
    
    return true;
}

double DrawdownMonitor::calculateDrawdownPercent(double currentValue, double highWaterMark) const {
    if (highWaterMark <= 0.0) {
        return 0.0;
    }
    
    return std::max(0.0, (highWaterMark - currentValue) / highWaterMark);
}

DrawdownEvent DrawdownMonitor::createDrawdownEvent(const StrategyId& strategyId, double drawdownPercent, 
                                                 double highWaterMark, double currentValue, Timestamp hwmTime, 
                                                 bool thresholdBreached) {
    DrawdownEvent event;
    event.strategyId = strategyId;
    event.drawdownPercent = drawdownPercent;
    event.highWaterMark = highWaterMark;
    event.currentValue = currentValue;
    event.highWaterMarkTime = hwmTime;
    event.eventTime = std::chrono::system_clock::now();
    event.isCritical = thresholdBreached;
    return event;
}

void DrawdownMonitor::updateHighWaterMark(DrawdownInfo& drawdownInfo, double currentValue) {
    drawdownInfo.highWaterMark.value = currentValue;
    drawdownInfo.highWaterMark.timestamp = std::chrono::system_clock::now();
    drawdownInfo.drawdownPercent = 0.0;
    drawdownInfo.warningThresholdBreached = false;
    drawdownInfo.criticalThresholdBreached = false;
}

void DrawdownMonitor::pruneHistory() {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    
    // Calculate cutoff time based on lookback period
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::minutes(m_drawdownConfig.lookbackPeriodMinutes);
    
    // Prune portfolio history
    while (!m_portfolioValueHistory.empty() && m_portfolioValueHistory.front().first < cutoff) {
        m_portfolioValueHistory.pop_front();
    }
    
    // Prune strategy histories
    for (auto& [strategyId, history] : m_strategyValueHistory) {
        while (!history.empty() && history.front().first < cutoff) {
            history.pop_front();
        }
    }
}

} // namespace risk
} // namespace order_imbalance_system