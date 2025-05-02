/**
 * Unified pre-trade risk filter that combines all pre-trade checks
 *
 * File: pre_trade_risk_filter.hpp
 * Created: 2025-04-29
 */

 #pragma once

 #include "../risk_types.hpp"
 #include "../risk_config.hpp"
 #include "position_limit_checker.hpp"
 #include "exposure_calculator.hpp"
 #include "order_validator.hpp"
 #include "fat_finger_protection.hpp"
 
 #include <memory>
 #include <vector>
 #include <string>
 
 namespace risk {
 
 /**
  * PreTradeRiskFilter orchestrates all pre-trade risk checks.
  * It validates trading signals before they become orders.
  */
 class PreTradeRiskFilter {
 public:
     /**
      * Constructor
      * @param config Risk configuration for this filter
      */
     explicit PreTradeRiskFilter(const RiskConfig& config);
     
     /**
      * Initialize the filter and its components
      * @return true if initialization was successful, false otherwise
      */
     bool initialize();
     
     /**
      * Validate a trading signal
      * @param signal The trading signal to validate
      * @return ValidationResult containing approval status and any violations
      */
     ValidationResult validateSignal(const TradingSignal& signal);
     
     /**
      * Validate an order request
      * @param orderRequest The order request to validate
      * @return ValidationResult containing approval status and any violations
      */
     ValidationResult validateOrder(const OrderRequest& orderRequest);
     
     /**
      * Update risk parameters
      * @param params The new risk parameters
      * @return true if update was successful, false otherwise
      */
     bool updateRiskParameters(const RiskParameters& params);
     
     /**
      * Get the current risk parameters
      * @param exchangeId The exchange ID
      * @param tradingPair The trading pair
      * @param strategyId The strategy ID
      * @return RiskParameters containing current risk limits
      */
     RiskParameters getRiskParameters(const ExchangeId& exchangeId, 
                                     const TradingPair& tradingPair,
                                     const StrategyId& strategyId) const;
     
     /**
      * Set the active mode for the risk filter
      * @param mode The new risk manager mode
      */
     void setMode(RiskManagerMode mode);
     
     /**
      * Get the active mode for the risk filter
      * @return The current risk manager mode
      */
     RiskManagerMode getMode() const;
 
 private:
     // Sub-components
     std::unique_ptr<PositionLimitChecker> m_positionLimitChecker;
     std::unique_ptr<ExposureCalculator> m_exposureCalculator;
     std::unique_ptr<OrderValidator> m_orderValidator;
     std::unique_ptr<FatFingerProtection> m_fatFingerProtection;
     
     // Configuration and state
     RiskConfig m_config;
     RiskManagerMode m_activeMode;
     
     // Helper methods
     ValidationResult performAllChecks(const TradingSignal& signal);
     ValidationResult performAllChecks(const OrderRequest& orderRequest);
     bool passesBasicValidation(const TradingSignal& signal, std::vector<RiskViolation>& violations);
     bool passesBasicValidation(const OrderRequest& orderRequest, std::vector<RiskViolation>& violations);
 };
 
 } // namespace risk

