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
 
 namespace trading{
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
     double minFreeCollateralRatio;
     double targetFreeCollateralRatio;
     bool enableEmergencyFunding;
     std::vector<std::string> fundingSources;
     double maxFundingAmount;
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
     CollateralConfig collateralConfig; // Add this member to hold collateral configuration

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
     
     /**
      * Get position limit configuration
      * @return PositionLimitConfig object
      */
     const PositionLimitConfig& getPositionLimitConfig() const;
     
     /**
      * Set position limit configuration
      * @param config The new position limit configuration
      */
     void setPositionLimitConfig(const PositionLimitConfig& config);
     
     /**
      * Get exposure limit configuration
      * @return ExposureLimitConfig object
      */
     const ExposureLimitConfig& getExposureLimitConfig() const;
     
     /**
      * Set exposure limit configuration
      * @param config The new exposure limit configuration
      */
     void setExposureLimitConfig(const ExposureLimitConfig& config);
     
     /**
      * Get circuit breaker configuration
      * @return CircuitBreakerConfig object
      */
     const CircuitBreakerConfig& getCircuitBreakerConfig() const;
     
     /**
      * Set circuit breaker configuration
      * @param config The new circuit breaker configuration
      */
     void setCircuitBreakerConfig(const CircuitBreakerConfig& config);
     
     /**
      * Get stop loss configuration
      * @return StopLossConfig object
      */
     const StopLossConfig& getStopLossConfig() const;
     
     /**
      * Set stop loss configuration
      * @param config The new stop loss configuration
      */
     void setStopLossConfig(const StopLossConfig& config);
     
     /**
      * Get drawdown configuration
      * @return DrawdownConfig object
      */
     const DrawdownConfig& getDrawdownConfig() const;
     
     /**
      * Set drawdown configuration
      * @param config The new drawdown configuration
      */
     void setDrawdownConfig(const DrawdownConfig& config);
     
     /**
      * Get fat finger configuration
      * @return FatFingerConfig object
      */
     const FatFingerConfig& getFatFingerConfig() const;
     
     /**
      * Set fat finger configuration
      * @param config The new fat finger configuration
      */
     void setFatFingerConfig(const FatFingerConfig& config);
     
     /**
      * Get order validation configuration
      * @return OrderValidationConfig object
      */
     const OrderValidationConfig& getOrderValidationConfig() const;
     
     /**
      * Set order validation configuration
      * @param config The new order validation configuration
      */
     void setOrderValidationConfig(const OrderValidationConfig& config);
     
     /**
      * Get balance reconciliation configuration
      * @return BalanceReconciliationConfig object
      */
     const BalanceReconciliationConfig& getBalanceReconciliationConfig() const;
     
     /**
      * Set balance reconciliation configuration
      * @param config The new balance reconciliation configuration
      */
     void setBalanceReconciliationConfig(const BalanceReconciliationConfig& config);
     
     /**
      * Get rebalancing configuration
      * @return RebalancingConfig object
      */
     const RebalancingConfig& getRebalancingConfig() const;
     
     /**
      * Set rebalancing configuration
      * @param config The new rebalancing configuration
      */
     void setRebalancingConfig(const RebalancingConfig& config);
     
     /**
      * Get collateral configuration
      * @return CollateralConfig object
      */
     const CollateralConfig& getCollateralConfig() const;
     
     /**
      * Set collateral configuration
      * @param config The new collateral configuration
      */
     void setCollateralConfig(const CollateralConfig& config);
     
     /**
      * Get logging configuration
      * @return RiskLoggingConfig object
      */
     const RiskLoggingConfig& getLoggingConfig() const;
     
     /**
      * Set logging configuration
      * @param config The new logging configuration
      */
     void setLoggingConfig(const RiskLoggingConfig& config);
 
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
     RiskLoggingConfig m_loggingConfig;
     
     bool loadJsonConfiguration(const std::string& jsonStr);
 };
 
 } // namespace risk
    } // namespace trading

