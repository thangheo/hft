#include "risk/posttrade/balance_reconciliation.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <thread>

namespace trading {
namespace risk {

BalanceReconciliation::BalanceReconciliation(const RiskConfig& config, 
                                           std::shared_ptr<PositionTracker> positionTracker,
                                           std::shared_ptr<DataInterface> dataInterface,
                                           std::shared_ptr<ExecutionInterface> executionInterface)
    : m_positionTracker(positionTracker),
      m_dataInterface(dataInterface),
      m_executionInterface(executionInterface),
      m_config(config),
      m_reconciliationConfig(config.balanceReconciliationConfig),
      m_consecutiveFailures(0),
      m_reconciliationScheduled(false) {
}

bool BalanceReconciliation::initialize() {
    // Initialize last reconciliation result
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_lastReconciliation.success = true;
        m_lastReconciliation.matchedPositions = 0;
        m_lastReconciliation.mismatchedPositions = 0;
        m_lastReconciliation.discrepancies.clear();
        m_lastReconciliation.timestamp = std::chrono::system_clock::now();
    }
    
    // Schedule regular reconciliations if enabled
    if (m_reconciliationConfig.enableScheduledReconciliation) {
        return scheduleReconciliations();
    }
    
    return true;
}

ReconciliationResult BalanceReconciliation::reconcileAllPositions() {
    ReconciliationResult result;
    result.matchedPositions = 0;
    result.mismatchedPositions = 0;
    result.discrepancies.clear();
    result.timestamp = std::chrono::system_clock::now();
    
    // Get all exchanges
    auto exchanges = m_positionTracker->getTrackingExchanges();
    
    // Reconcile positions for each exchange
    for (const auto& exchangeId : exchanges) {
        auto exchangeResult = reconcileExchangePositions(exchangeId);
        
        // Aggregate results
        result.matchedPositions += exchangeResult.matchedPositions;
        result.mismatchedPositions += exchangeResult.mismatchedPositions;
        result.discrepancies.insert(result.discrepancies.end(), 
                                  exchangeResult.discrepancies.begin(), 
                                  exchangeResult.discrepancies.end());
    }
    
    // Set success flag
    result.success = (result.mismatchedPositions == 0);
    
    // Update consecutive failures count
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (!result.success) {
            m_consecutiveFailures++;
        } else {
            m_consecutiveFailures = 0;
        }
        
        m_lastReconciliation = result;
    }
    
    return result;
}

ReconciliationResult BalanceReconciliation::reconcileExchangePositions(const ExchangeId& exchangeId) {
    ReconciliationResult result;
    result.matchedPositions = 0;
    result.mismatchedPositions = 0;
    result.discrepancies.clear();
    result.timestamp = std::chrono::system_clock::now();
    
    // Get expected positions from position tracker
    std::vector<Position> expectedPositions = m_positionTracker->getExchangePositions(exchangeId);
    
    // Get actual positions from exchange
    std::map<std::string, double> actualPositions;
    bool fetchSuccess = fetchActualPositions(exchangeId, actualPositions);
    
    if (!fetchSuccess) {
        // Failed to fetch actual positions
        result.success = false;
        return result;
    }
    
    // Compare expected vs actual positions
    for (const auto& position : expectedPositions) {
        const std::string& tradingPair = position.tradingPair;
        
        // Skip zero positions
        if (std::abs(position.quantity) < 1e-8) {
            continue;
        }
        
        // Find actual position
        auto it = actualPositions.find(tradingPair);
        if (it == actualPositions.end()) {
            // Trading pair not found in actual positions
            double actualQuantity = 0.0;
            double discrepancy = position.quantity - actualQuantity;
            
            // Create reconciliation event
            auto event = createReconciliationEvent(exchangeId, tradingPair,
                                                position.quantity, actualQuantity, discrepancy,
                                                shouldAutoCorrect(std::abs(discrepancy / position.quantity)));
            
            // Apply auto-correction if configured
            if (event.autoCorrect) {
                applyCorrection(exchangeId, tradingPair, actualQuantity);
            }
            
            // Add to discrepancies
            result.discrepancies.push_back(event);
            result.mismatchedPositions++;
            
            // Notify callbacks
            notifyReconciliationEvent(event);
        } else {
            double actualQuantity = it->second;
            double discrepancy = position.quantity - actualQuantity;
            
            // Check if discrepancy exceeds tolerance
            if (std::abs(discrepancy) > m_reconciliationConfig.positionDiscrepancyTolerance) {
                // Create reconciliation event
                auto event = createReconciliationEvent(exchangeId, tradingPair,
                                                    position.quantity, actualQuantity, discrepancy,
                                                    shouldAutoCorrect(std::abs(discrepancy / position.quantity)));
                
                // Apply auto-correction if configured
                if (event.autoCorrect) {
                    applyCorrection(exchangeId, tradingPair, actualQuantity);
                }
                
                // Add to discrepancies
                result.discrepancies.push_back(event);
                result.mismatchedPositions++;
                
                // Notify callbacks
                notifyReconciliationEvent(event);
            } else {
                // Position matches within tolerance
                result.matchedPositions++;
            }
            
            // Remove from actual positions map to track unaccounted positions
            actualPositions.erase(it);
        }
    }
    
    // Check for positions in actual but not in expected
    for (const auto& [tradingPair, actualQuantity] : actualPositions) {
        // Skip zero positions
        if (std::abs(actualQuantity) < 1e-8) {
            continue;
        }
        
        // This position exists in exchange but not in our tracker
        double expectedQuantity = 0.0;
        double discrepancy = expectedQuantity - actualQuantity;
        
        // Create reconciliation event
        auto event = createReconciliationEvent(exchangeId, tradingPair,
                                           expectedQuantity, actualQuantity, discrepancy,
                                           shouldAutoCorrect(1.0)); // 100% discrepancy
        
        // Apply auto-correction if configured
        if (event.autoCorrect) {
            applyCorrection(exchangeId, tradingPair, actualQuantity);
        }
        
        // Add to discrepancies
        result.discrepancies.push_back(event);
        result.mismatchedPositions++;
        
        // Notify callbacks
        notifyReconciliationEvent(event);
    }
    
    // Set success flag
    result.success = (result.mismatchedPositions == 0);
    
    return result;
}

bool BalanceReconciliation::scheduleReconciliations() {
    if (!m_reconciliationConfig.enableScheduledReconciliation) {
        return false;
    }
    
    // Mark as scheduled
    m_reconciliationScheduled = true;
    
    // Schedule the first reconciliation
    m_nextScheduledReconciliation = std::chrono::system_clock::now() + 
                                  std::chrono::minutes(m_reconciliationConfig.reconciliationIntervalMinutes);
    
    // Start a thread for periodic reconciliation
    std::thread([this]() {
        while (m_reconciliationScheduled) {
            // Sleep until next scheduled reconciliation
            auto now = std::chrono::system_clock::now();
            if (now < m_nextScheduledReconciliation) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                continue;
            }
            
            // Perform reconciliation
            reconcileAllPositions();
            
            // Schedule next reconciliation
            m_nextScheduledReconciliation = std::chrono::system_clock::now() + 
                                          std::chrono::minutes(m_reconciliationConfig.reconciliationIntervalMinutes);
        }
    }).detach();
    
    return true;
}

bool BalanceReconciliation::cancelScheduledReconciliations() {
    m_reconciliationScheduled = false;
    return true;
}

bool BalanceReconciliation::updateConfiguration(const BalanceReconciliationConfig& reconciliationConfig) {
    // Validate configuration
    if (reconciliationConfig.reconciliationIntervalMinutes <= 0 ||
        reconciliationConfig.positionDiscrepancyTolerance < 0 ||
        reconciliationConfig.autoCorrectThresholdPercent < 0 ||
        reconciliationConfig.autoCorrectThresholdPercent > 100) {
        return false;
    }
    
    bool wasScheduled = m_reconciliationConfig.enableScheduledReconciliation;
    m_reconciliationConfig = reconciliationConfig;
    
    // Update reconciliation schedule if needed
    if (wasScheduled != reconciliationConfig.enableScheduledReconciliation) {
        if (reconciliationConfig.enableScheduledReconciliation) {
            scheduleReconciliations();
        } else {
            cancelScheduledReconciliations();
        }
    }
    
    return true;
}

BalanceReconciliationConfig BalanceReconciliation::getConfiguration() const {
    return m_reconciliationConfig;
}

void BalanceReconciliation::registerReconciliationCallback(
    const std::function<void(const PositionReconciliationEvent&)>& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_reconciliationCallbacks.push_back(callback);
}

ReconciliationResult BalanceReconciliation::getLastReconciliationResult() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_lastReconciliation;
}

bool BalanceReconciliation::correctDiscrepancy(const ExchangeId& exchangeId, const TradingPair& tradingPair, double correctQuantity) {
    return applyCorrection(exchangeId, tradingPair, correctQuantity);
}

PositionReconciliationEvent BalanceReconciliation::createReconciliationEvent(const ExchangeId& exchangeId, 
                                                                         const TradingPair& tradingPair,
                                                                         double expected, double actual, double discrepancy,
                                                                         bool autoCorrect) {
    PositionReconciliationEvent event;
    event.exchangeId = exchangeId;
    event.tradingPair = tradingPair;
    event.expectedQuantity = expected;
    event.actualQuantity = actual;
    event.discrepancy = discrepancy;
    event.discrepancyPercent = expected != 0 ? std::abs(discrepancy / expected) * 100.0 : 100.0;
    event.autoCorrect = autoCorrect;
    event.timestamp = std::chrono::system_clock::now();
    return event;
}

bool BalanceReconciliation::fetchActualPositions(const ExchangeId& exchangeId, std::map<std::string, double>& actualPositions) {
    try {
        // Get positions from data interface
        auto positions = m_dataInterface->getExchangePositions(exchangeId);
        
        // Convert to map for easier lookup
        for (const auto& position : positions) {
            actualPositions[position.first] = position.second;
        }
        
        return true;
    } catch (const std::exception& e) {
        // Log error
        // TODO: Add proper logging
        
        return false;
    }
}

void BalanceReconciliation::notifyReconciliationEvent(const PositionReconciliationEvent& event) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (const auto& callback : m_reconciliationCallbacks) {
        callback(event);
    }
}

bool BalanceReconciliation::shouldAutoCorrect(double discrepancyPercent) const {
    return m_reconciliationConfig.enableAutoCorrection && 
           discrepancyPercent * 100.0 <= m_reconciliationConfig.autoCorrectThresholdPercent;
}

bool BalanceReconciliation::applyCorrection(const ExchangeId& exchangeId, const TradingPair& tradingPair, double actualQuantity) {
    try {
        // Update position in tracker
        m_positionTracker->overridePosition(exchangeId, tradingPair, actualQuantity);
        return true;
    } catch (const std::exception& e) {
        // Log error
        // TODO: Add proper logging
        return false;
    }
}

} // namespace risk
} // namespace trading