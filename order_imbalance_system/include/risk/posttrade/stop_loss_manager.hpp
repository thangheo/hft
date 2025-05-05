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

namespace trading {
namespace risk {

/**
 * Structure to hold stop loss information for a position
 */
struct StopLossInfo {
    double stopPrice;
    double highestPrice; // For trailing stops
    double lowestPrice;  // For trailing stops
    bool isTrailing;
    double trailingDistance;
    bool active;
    Timestamp lastUpdated;
};

/**
 * StopLossManager implements and monitors stop-loss orders.
 * It manages trailing stops and can automatically liquidate positions when stop levels are reached.
 */
class StopLossManager {
public:
    /**
     * Constructor
     * @param config Risk configuration containing stop loss settings
     * @param positionTracker Reference to position tracker
     * @param dataInterface Interface to access market data
     * @param executionInterface Interface to execution system
     */
    StopLossManager(const RiskConfig& config, 
                   std::shared_ptr<PositionTracker> positionTracker,
                   std::shared_ptr<DataInterface> dataInterface,
                   std::shared_ptr<ExecutionInterface> executionInterface);
    
    /**
     * Initialize the stop loss manager
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Process an execution report
     * @param executionReport The execution report to process
     */
    void processExecutionReport(const ExecutionReport& executionReport);
    
    /**
     * Update market data for stop loss evaluation
     * @param marketData The new market data
     */
    void updateMarketData(const MarketData& marketData);
    
    /**
     * Set a stop loss for a position
     * @param exchangeId Exchange identifier
     * @param tradingPair Trading pair
     * @param stopPrice Stop price
     * @param isTrailing Whether this is a trailing stop
     * @param trailingDistance Distance for trailing stops
     * @return true if stop loss was set successfully, false otherwise
     */
    bool setStopLoss(const ExchangeId& exchangeId, 
                    const TradingPair& tradingPair, 
                    double stopPrice, 
                    bool isTrailing = false,
                    double trailingDistance = 0.0);
    
    /**
     * Remove a stop loss for a position
     * @param exchangeId Exchange identifier
     * @param tradingPair Trading pair
     * @return true if stop loss was removed successfully, false otherwise
     */
    bool removeStopLoss(const ExchangeId& exchangeId, const TradingPair& tradingPair);
    
    /**
     * Get stop loss information for a position
     * @param exchangeId Exchange identifier
     * @param tradingPair Trading pair
     * @return Optional stop loss information, or nullopt if no stop loss exists
     */
    std::optional<StopLossInfo> getStopLoss(const ExchangeId& exchangeId, const TradingPair& tradingPair) const;
    
    /**
     * Check all positions against their stop losses
     * @return Vector of triggered stop loss events
     */
    std::vector<StopLossEvent> checkStopLosses();
    
    /**
     * Liquidate a position due to a triggered stop loss
     * @param exchangeId Exchange identifier
     * @param tradingPair Trading pair
     * @return true if liquidation was initiated successfully, false otherwise
     */
    bool liquidatePosition(const ExchangeId& exchangeId, const TradingPair& tradingPair);
    
    /**
     * Update stop loss configuration
     * @param stopLossConfig New stop loss configuration
     * @return true if update was successful, false otherwise
     */
    bool updateConfiguration(const StopLossConfig& stopLossConfig);
    
    /**
     * Get the current stop loss configuration
     * @return The current configuration
     */
    StopLossConfig getConfiguration() const;
    
    /**
     * Register a callback for stop loss events
     * @param callback The function to call when stop loss events occur
     */
    void registerStopLossCallback(const std::function<void(const StopLossEvent&)>& callback);
    
private:
    std::shared_ptr<PositionTracker> m_positionTracker;
    std::shared_ptr<DataInterface> m_dataInterface;
    std::shared_ptr<ExecutionInterface> m_executionInterface;
    RiskConfig m_config;
    StopLossConfig m_stopLossConfig;
    
    // Stop loss tracking
    mutable std::mutex m_stopLossMutex;
    std::map<std::pair<std::string, std::string>, StopLossInfo> m_stopLosses; // Key: <exchangeId, tradingPair>
    
    // Market data cache
    mutable std::mutex m_marketDataMutex;
    std::unordered_map<std::string, double> m_lastPrices;
    
    // Callbacks
    mutable std::mutex m_callbackMutex;
    std::vector<std::function<void(const StopLossEvent&)>> m_stopLossCallbacks;
    
    // Helper methods
    void updateTrailingStops(const std::string& symbol, double price);
    bool isStopLossTriggered(const StopLossInfo& stopLoss, double currentPrice, double positionSize);
    StopLossEvent createStopLossEvent(const ExchangeId& exchangeId, const TradingPair& tradingPair,
                                     double positionSize, double entryPrice, double currentPrice, double lossPercentage);
};

} // namespace risk
} // namespace trading