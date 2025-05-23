/**
 * Risk configuration structures and parameter definitions
 *
 * File: risk_config.hpp
 * Created: 2025-04-29
 */

#pragma once

#include "risk_types.hpp"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <optional>

namespace trading {
namespace risk {

/**
 * Configuration structure for position limits
 */
struct PositionLimitConfig {
    std::map<std::string, double> assetLimits;
    std::map<std::string, double> strategyLimits;
    std::map<std::string, double> exchangeLimits;
    double defaultLimit;
    bool enforceStrictly;
};

/**
 * Configuration structure for exposure limits
 */
struct ExposureLimitConfig {
    double maxTotalExposure;
    double maxAssetExposure;
    double maxStrategyExposure;
    double maxExchangeExposure;
    double volatilityMultiplier;
    bool includeUnrealizedPnl;
};

/**
 * Configuration structure for circuit breakers
 */
struct CircuitBreakerConfig {
    std::map<std::string, double> warningThresholds;
    std::map<std::string, double> criticalThresholds;
    std::chrono::seconds coolingPeriod;
    std::chrono::seconds warmupPeriod;
    bool autoResume;
    int maxTriggersPerDay;
};

/**
 * Configuration structure for stop loss
 */
struct StopLossConfig {
    double maxLossPercentage;
    double trailingStopDistance;
    bool enableTrailingStop;
    bool forceLiquidation;
    std::chrono::seconds liquidationDelay;
};

/**
 * Configuration structure for drawdown monitoring
 */
struct DrawdownConfig {
    double warningThreshold;
    double criticalThreshold;
    std::chrono::hours lookbackPeriod;
    bool enableAutoRiskReduction;
    double riskReductionFactor;
};

/**
 * Configuration structure for fat finger protection
 */
struct FatFingerConfig {
    double maxPriceDeviation;
    double maxOrderSize;
    double typicalOrderSize;
    double notionalValueLimit;
    bool enforceAdditionalVerification;
};

/**
 * Configuration structure for order validation
 */
struct OrderValidationConfig {
    bool validateMinOrderSize;
    bool validatePriceRange;
    std::map<ExchangeId, std::map<TradingPair, double>> minOrderSizes;
    double maxPriceDeviationPercent;
    bool enforceSymbolWhitelist;
    std::vector<std::string> symbolWhitelist;
};

/**
 * Configuration structure for balance reconciliation
 */
struct BalanceReconciliationConfig {
    std::chrono::minutes reconciliationFrequency;
    double maxDiscrepancyPercent;
    bool autoCorrectDiscrepancies;
    bool alertOnDiscrepancy;
    int maxConsecutiveFailures;
};

/**
 * Configuration structure for rebalancing
 */
struct RebalancingConfig {
    bool autoRebalancingEnabled = false;
    bool autoExecuteRebalancing = false;
    bool parallelExecution = false;
    int rebalancingIntervalMinutes = 60;
    double allocationDeviationThreshold = 0.05; // 5%
    double minimumOrderSize = 0.0;
    double minimumRebalancingValue = 0.0;
    double defaultTransferFeeRate = 0.001; // 0.1% default fee
    std::unordered_map<std::string, double> targetAllocations;
    std::unordered_map<std::string, double> fixedPrices;
    std::unordered_map<std::string, double> assetTransferFees;
    std::unordered_map<std::string, double> exchangeTransferFees;
    std::vector<std::string> preferredExchanges;
};

/**
 * Configuration structure for collateral management
 */
struct CollateralConfig {
    double targetFreeCollateralRatio;
    bool enableEmergencyFunding;
    std::vector<std::string> fundingSources;
    double maxFundingAmount;

    // Minimum ratio of free collateral to total collateral (e.g., 0.2 for 20%)
    double minimumFreeCollateralRatio = 0.2;

    // If free collateral ratio drops below this, emergency funding is triggered
    double emergencyFundingThreshold = 0.1;

    // If true, emergency funding is auto-initiated
    bool autoInitiateEmergencyFunding = false;

    // List of exchanges managed by the collateral manager
    std::vector<ExchangeId> managedExchanges;

    // Minimum balances required for each asset (asset symbol -> minimum amount)
    std::map<std::string, double> minimumBalances;

    // Buffer to add to emergency funding (e.g., 0.1 for 10% extra)
    double emergencyFundingBuffer = 0.1;

    // Source to use for emergency funding (e.g., "main_wallet")
    std::string emergencyFundingSource;

    // Margin rates per exchange and symbol (ExchangeId -> (symbol -> margin rate))
    std::map<ExchangeId, std::map<std::string, double>> exchangeMarginRates;

    // Base currency for collateral calculations (e.g., "USD")
    std::string baseCurrency = "USD";

    // Asset conversion rates (asset symbol -> conversion rate to base currency)
    std::map<std::string, double> assetConversionRates;

    // Minimums for critical assets (asset symbol -> minimum amount)
    std::map<std::string, double> criticalAssetMinimums;
};

/**
 * Configuration structure for portfolio management
 */
struct PortfolioConfig {
    bool autoRebalancingEnabled = false;
    bool autoExecuteRebalancing = false;
    bool enableRealTimeMetrics = true;
    bool enableAllocationTracking = true;
    std::chrono::minutes metricsUpdateInterval = std::chrono::minutes(5);
    double minimumPortfolioValue = 1000.0;
    std::string baseCurrency = "USD";
    std::vector<std::string> supportedAssets;
    std::unordered_map<std::string, double> targetAllocations;
    bool enableCrossExchangeAggregation = true;
    bool enablePortfolioRebalancing = true;
};

/**
 * Configuration structure for risk logging
 */
struct RiskLoggingConfig {
    std::string logLevel;
    std::string logDirectory;
    bool enableDetailedLogs;
    bool logToConsole;
    bool logToFile;
    std::chrono::days logRetentionPeriod;
};

/**
 * Main risk configuration container
 */
class RiskConfig {
public:
    /**
     * Constructor with file path
     * @param configFilePath Path to the configuration file
     */
    explicit RiskConfig(const std::string& configFilePath);
    
    /**
     * Default constructor
     */
    RiskConfig();
    
    /**
     * Load configuration from file
     * @param configFilePath Path to the configuration file
     * @return true if loading was successful, false otherwise
     */
    bool loadFromFile(const std::string& configFilePath);
    
    /**
     * Save configuration to file
     * @param configFilePath Path to save the configuration to
     * @return true if saving was successful, false otherwise
     */
    bool saveToFile(const std::string& configFilePath) const;
    
    /**
     * Validate the configuration
     * @return true if configuration is valid, false otherwise
     */
    bool validate() const;
    
    // Getters
    const PositionLimitConfig& getPositionLimitConfig() const;
    const ExposureLimitConfig& getExposureLimitConfig() const;
    const CircuitBreakerConfig& getCircuitBreakerConfig() const;
    const StopLossConfig& getStopLossConfig() const;
    const DrawdownConfig& getDrawdownConfig() const;
    const FatFingerConfig& getFatFingerConfig() const;
    const OrderValidationConfig& getOrderValidationConfig() const;
    const BalanceReconciliationConfig& getBalanceReconciliationConfig() const;
    const RebalancingConfig& getRebalancingConfig() const;
    const CollateralConfig& getCollateralConfig() const;
    const PortfolioConfig& getPortfolioConfig() const;
    const RiskLoggingConfig& getLoggingConfig() const;
    
    // Setters
    void setPositionLimitConfig(const PositionLimitConfig& config);
    void setExposureLimitConfig(const ExposureLimitConfig& config);
    void setCircuitBreakerConfig(const CircuitBreakerConfig& config);
    void setStopLossConfig(const StopLossConfig& config);
    void setDrawdownConfig(const DrawdownConfig& config);
    void setFatFingerConfig(const FatFingerConfig& config);
    void setOrderValidationConfig(const OrderValidationConfig& config);
    void setBalanceReconciliationConfig(const BalanceReconciliationConfig& config);
    void setRebalancingConfig(const RebalancingConfig& config);
    void setCollateralConfig(const CollateralConfig& config);
    void setPortfolioConfig(const PortfolioConfig& config);
    void setLoggingConfig(const RiskLoggingConfig& config);

    // Public member for direct access (as shown in original code)
    CollateralConfig collateralConfig;
    PortfolioConfig portfolioConfig;

private:
    PositionLimitConfig m_positionLimitConfig;
    ExposureLimitConfig m_exposureLimitConfig;
    CircuitBreakerConfig m_circuitBreakerConfig;
    StopLossConfig m_stopLossConfig;
    DrawdownConfig m_drawdownConfig;
    FatFingerConfig m_fatFingerConfig;
    OrderValidationConfig m_orderValidationConfig;
    BalanceReconciliationConfig m_balanceReconciliationConfig;
    RebalancingConfig m_rebalancingConfig;
    CollateralConfig m_collateralConfig;
    PortfolioConfig m_portfolioConfig;
    RiskLoggingConfig m_loggingConfig;
    
    bool loadJsonConfiguration(const std::string& jsonStr);
};

} // namespace risk
} // namespace trading