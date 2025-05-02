/**
 * Interface for verifying expected vs. actual balances
 *
 * File: balance_reconciliation.hpp
 * Created: 2025-04-29
 */
#pragma once

#include "../risk_types.hpp"
#include "../risk_events.hpp"
#include "../risk_config.hpp"
#include "../portfolio/position_tracker.hpp"
#include "../interfaces/data_interface.hpp"
#include "../interfaces/execution_interface.hpp"

#include <map>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace risk {

/**
 * Structure to hold reconciliation result information
 */
struct ReconciliationResult {
    bool success;
    int matchedPositions;
    int mismatchedPositions;
    std::vector<PositionReconciliationEvent> discrepancies;
    Timestamp timestamp;
};

/**
 * BalanceReconciliation verifies expected vs. actual balances.
 * It detects discrepancies in position tracking and triggers reconciliation processes.
 */
class BalanceReconciliation {
public:
    /**
     * Constructor
     * @param config Risk configuration containing reconciliation settings
     * @param positionTracker Reference to position tracker
     * @param dataInterface Interface to access exchange data
     * @param executionInterface Interface to execution system
     */
    BalanceReconciliation(const RiskConfig& config, 
                         std::shared_ptr<PositionTracker> positionTracker,
                         std::shared_ptr<DataInterface> dataInterface,
                         std::shared_ptr<ExecutionInterface> executionInterface);
    
    /**
     * Initialize the reconciliation manager
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Perform a reconciliation for all exchanges
     * @return The reconciliation result
     */
    ReconciliationResult reconcileAllPositions();
    
    /**
     * Perform a reconciliation for a specific exchange
     * @param exchangeId The exchange identifier
     * @return The reconciliation result
     */
    ReconciliationResult reconcileExchangePositions(const ExchangeId& exchangeId);
    
    /**
     * Schedule regular reconciliations based on configuration
     * @return true if scheduling was successful, false otherwise
     */
    bool scheduleReconciliations();
    
    /**
     * Cancel scheduled reconciliations
     * @return true if cancellation was successful, false otherwise
     */
    bool cancelScheduledReconciliations();
    
    /**
     * Update reconciliation configuration
     * @param reconciliationConfig New reconciliation configuration
     * @return true if update was successful, false otherwise
     */
    bool updateConfiguration(const BalanceReconciliationConfig& reconciliationConfig);
    
    /**
     * Get the current reconciliation configuration
     * @return The current configuration
     */
    BalanceReconciliationConfig getConfiguration() const;
    
    /**
     * Register a callback for reconciliation events
     * @param callback The function to call when reconciliation events occur
     */
    void registerReconciliationCallback(const std::function<void(const PositionReconciliationEvent&)>& callback);
    
    /**
     * Get the most recent reconciliation result
     * @return The most recent reconciliation result
     */
    ReconciliationResult getLastReconciliationResult() const;
    
    /**
     * Manually correct a discrepancy
     * @param exchangeId The exchange identifier
     * @param tradingPair The trading pair
     * @param correctQuantity The quantity to set
     * @return true if correction was successful, false otherwise
     */
    bool correctDiscrepancy(const ExchangeId& exchangeId, const TradingPair& tradingPair, double correctQuantity);

private:
    std::shared_ptr<PositionTracker> m_positionTracker;
    std::shared_ptr<DataInterface> m_dataInterface;
    std::shared_ptr<ExecutionInterface> m_executionInterface;
    RiskConfig m_config;
    BalanceReconciliationConfig m_reconciliationConfig;
    
    // Reconciliation state
    mutable std::mutex m_stateMutex;
    ReconciliationResult m_lastReconciliation;
    int m_consecutiveFailures;
    std::chrono::system_clock::time_point m_nextScheduledReconciliation;
    bool m_reconciliationScheduled;
    
    // Callbacks
    mutable std::mutex m_callbackMutex;
    std::vector<std::function<void(const PositionReconciliationEvent&)>> m_reconciliationCallbacks;
    
    // Helper methods
    PositionReconciliationEvent createReconciliationEvent(const ExchangeId& exchangeId, const TradingPair& tradingPair,
                                                        double expected, double actual, double discrepancy,
                                                        bool autoCorrect);
    bool fetchActualPositions(const ExchangeId& exchangeId, std::map<std::string, double>& actualPositions);
    void notifyReconciliationEvent(const PositionReconciliationEvent& event);
    bool shouldAutoCorrect(double discrepancyPercent) const;
    bool applyCorrection(const ExchangeId& exchangeId, const TradingPair& tradingPair, double actualQuantity);
};

} // namespace risk
