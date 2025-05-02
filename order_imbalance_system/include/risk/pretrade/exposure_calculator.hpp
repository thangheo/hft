/**
 * Component that calculates and monitors total exposure across exchanges
 *
 * File: exposure_calculator.hpp
 * Created: 2025-04-29
 */
#pragma once

#include "../risk_types.hpp"
#include "../risk_config.hpp"
#include "../portfolio/position_tracker.hpp"
#include "../interfaces/data_interface.hpp"

#include <map>
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>

namespace risk {

/**
 * ExposureCalculator calculates and monitors the total exposure across exchanges.
 * It enforces exposure limits at multiple levels (asset, exchange, portfolio).
 */
class ExposureCalculator {
public:
    /**
     * Constructor
     * @param config Risk configuration containing exposure limits
     * @param positionTracker Reference to the position tracker to get current positions
     * @param dataInterface Interface to access market data
     */
    ExposureCalculator(const RiskConfig& config, 
                      std::shared_ptr<PositionTracker> positionTracker,
                      std::shared_ptr<DataInterface> dataInterface);
    
    /**
     * Initialize the calculator
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Check if adding a new position would exceed exposure limits
     * @param signal The trading signal to check
     * @param violations Vector to fill with any violations
     * @return true if within limits, false otherwise
     */
    bool checkExposureLimit(const TradingSignal& signal, std::vector<RiskViolation>& violations);
    
    /**
     * Check if adding a new position would exceed exposure limits
     * @param orderRequest The order request to check
     * @param violations Vector to fill with any violations
     * @return true if within limits, false otherwise
     */
    bool checkExposureLimit(const OrderRequest& orderRequest, std::vector<RiskViolation>& violations);
    
    /**
     * Update market data used for exposure calculations
     * @param marketData New market data
     */
    void updateMarketData(const MarketData& marketData);
    
    /**
     * Calculate the total exposure across all positions
     * @return The total exposure value
     */
    double calculateTotalExposure() const;
    
    /**
     * Calculate exposure for a specific asset
     * @param assetId The asset identifier
     * @return The asset exposure value
     */
    double calculateAssetExposure(const std::string& assetId) const;
    
    /**
     * Calculate exposure for a specific strategy
     * @param strategyId The strategy identifier
     * @return The strategy exposure value
     */
    double calculateStrategyExposure(const StrategyId& strategyId) const;
    
    /**
     * Calculate exposure for a specific exchange
     * @param exchangeId The exchange identifier
     * @return The exchange exposure value
     */
    double calculateExchangeExposure(const ExchangeId& exchangeId) const;
    
    /**
     * Update exposure limits
     * @param exposureLimits New exposure limits configuration
     * @return true if update was successful, false otherwise
     */
    bool updateExposureLimits(const ExposureLimitConfig& exposureLimits);
    
    /**
     * Get the current exposure limits
     * @return The current exposure limits configuration
     */
    ExposureLimitConfig getExposureLimits() const;

private:
    std::shared_ptr<PositionTracker> m_positionTracker;
    std::shared_ptr<DataInterface> m_dataInterface;
    RiskConfig m_config;
    ExposureLimitConfig m_exposureLimits;
    
    // Cache for market data
    mutable std::mutex m_marketDataMutex;
    std::unordered_map<std::string, double> m_prices;
    std::unordered_map<std::string, double> m_volatilities;
    
    // Cache for exposure calculations
    mutable std::mutex m_exposureCacheMutex;
    mutable std::unordered_map<std::string, double> m_assetExposureCache;
    mutable std::unordered_map<std::string, double> m_strategyExposureCache;
    mutable std::unordered_map<std::string, double> m_exchangeExposureCache;
    mutable double m_totalExposureCache;
    mutable bool m_exposureCacheValid;
    
    // Helper methods
    double calculatePositionExposure(const Position& position) const;
    double getPrice(const std::string& symbol) const;
    double getVolatility(const std::string& symbol) const;
    void invalidateCache();
    double calculateNotionalValue(const TradingPair& tradingPair, double quantity) const;
};

} // namespace risk

