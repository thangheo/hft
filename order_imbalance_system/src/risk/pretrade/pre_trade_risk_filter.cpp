/**
 * Implementation of the pre-trade risk filter
 *
 * File: pre_trade_risk_filter.cpp
 * Created: 2025-04-29
 */

 #include "risk/pretrade/pre_trade_risk_filter.hpp"
 #include "common/logger.hpp"
 
 #include <chrono>
 
 namespace trading {
 namespace risk {
 
 PreTradeRiskFilter::PreTradeRiskFilter(const RiskConfig& config)
     : m_config(config)
     , m_activeMode(RiskManagerMode::NORMAL)
 {
     // Components will be initialized in initialize()
 }
 
 bool PreTradeRiskFilter::initialize() {
     try {
         LOG_INFO("Initializing pre-trade risk filter...");
         
         // Create shared dependencies
         auto positionTracker = std::make_shared<PositionTracker>(m_config);
         auto dataInterface = std::make_shared<DataInterface>(m_config);
         
         // Initialize position tracker and data interface
         if (!positionTracker->initialize()) {
             LOG_ERROR("Failed to initialize position tracker");
             return false;
         }
         
         if (!dataInterface->initialize()) {
             LOG_ERROR("Failed to initialize data interface");
             return false;
         }
         
         // Create and initialize each component
         m_positionLimitChecker = std::make_unique<PositionLimitChecker>(m_config, positionTracker);
         if (!m_positionLimitChecker->initialize()) {
             LOG_ERROR("Failed to initialize position limit checker");
             return false;
         }
         
         m_exposureCalculator = std::make_unique<ExposureCalculator>(m_config, positionTracker, dataInterface);
         if (!m_exposureCalculator->initialize()) {
             LOG_ERROR("Failed to initialize exposure calculator");
             return false;
         }
         
         m_orderValidator = std::make_unique<OrderValidator>(m_config, dataInterface);
         if (!m_orderValidator->initialize()) {
             LOG_ERROR("Failed to initialize order validator");
             return false;
         }
         
         m_fatFingerProtection = std::make_unique<FatFingerProtection>(m_config, dataInterface);
         if (!m_fatFingerProtection->initialize()) {
             LOG_ERROR("Failed to initialize fat finger protection");
             return false;
         }
         
         LOG_INFO("Pre-trade risk filter initialized successfully");
         return true;
     } catch (const std::exception& ex) {
         LOG_ERROR("Failed to initialize pre-trade risk filter: {}", ex.what());
         return false;
     }
 }
 
 ValidationResult PreTradeRiskFilter::validateSignal(const TradingSignal& signal) {
     // Start timing the validation
     auto startTime = std::chrono::high_resolution_clock::now();
     
     LOG_DEBUG("Validating signal: id={}, pair={}, direction={}, size={}",
               signal.id, signal.tradingPair.getSymbol(), 
               (signal.direction == Direction::BUY ? "BUY" : "SELL"), 
               signal.targetSize);
     
     // Check if we're in emergency mode - reject all signals
     if (m_activeMode == RiskManagerMode::EMERGENCY) {
         ValidationResult result;
         result.signalId = signal.id;
         result.approved = false;
         
         RiskViolation violation;
         violation.violationType = "EMERGENCY_MODE_ACTIVE";
         violation.description = "Risk system is in emergency mode, all signals rejected";
         violation.limitValue = 0;
         violation.actualValue = 0;
         result.violations.push_back(violation);
         
         LOG_WARN("Signal {} rejected: risk system in emergency mode", signal.id);
         return result;
     }
     
     // Perform validation checks with normal or heightened thresholds
     return performAllChecks(signal);
 }
 
 ValidationResult PreTradeRiskFilter::validateOrder(const OrderRequest& orderRequest) {
     // Start timing the validation
     auto startTime = std::chrono::high_resolution_clock::now();
     
     LOG_DEBUG("Validating order: id={}, pair={}, side={}, type={}, quantity={}",
               orderRequest.clientOrderId, orderRequest.tradingPair.getSymbol(),
               (orderRequest.side == OrderSide::BUY ? "BUY" : "SELL"),
               static_cast<int>(orderRequest.type), orderRequest.quantity);
     
     // Check if we're in emergency mode - reject all orders
     if (m_activeMode == RiskManagerMode::EMERGENCY) {
         ValidationResult result;
         result.signalId = orderRequest.clientOrderId;
         result.approved = false;
         
         RiskViolation violation;
         violation.violationType = "EMERGENCY_MODE_ACTIVE";
         violation.description = "Risk system is in emergency mode, all orders rejected";
         violation.limitValue = 0;
         violation.actualValue = 0;
         result.violations.push_back(violation);
         
         LOG_WARN("Order {} rejected: risk system in emergency mode", orderRequest.clientOrderId);
         return result;
     }
     
     // Perform validation checks with normal or heightened thresholds
     return performAllChecks(orderRequest);
 }
 
 bool PreTradeRiskFilter::updateRiskParameters(const RiskParameters& params) {
     try {
         LOG_INFO("Updating risk parameters for {}/{}/{}",
                 params.exchangeId, params.tradingPair, params.strategyId);
         
         // Update position limits if provided
         if (params.maxPositionSize > 0) {
             if (!m_positionLimitChecker->updatePositionLimit(
                     params.tradingPair.getBaseAsset(), params.strategyId, params.maxPositionSize)) {
                 LOG_ERROR("Failed to update position limit");
                 return false;
             }
         }
         
         // Update exposure limits if provided
         if (params.maxExposure > 0) {
             ExposureLimitConfig exposureLimits = m_exposureCalculator->getExposureLimits();
             
             // Update relevant limits
             if (!params.strategyId.empty()) {
                 exposureLimits.strategyExposureLimits[params.strategyId] = params.maxExposure;
             } else if (!params.exchangeId.empty()) {
                 exposureLimits.exchangeExposureLimits[params.exchangeId] = params.maxExposure;
             } else if (!params.tradingPair.getSymbol().empty()) {
                 exposureLimits.assetExposureLimits[params.tradingPair.getBaseAsset()] = params.maxExposure;
             }
             
             if (!m_exposureCalculator->updateExposureLimits(exposureLimits)) {
                 LOG_ERROR("Failed to update exposure limits");
                 return false;
             }
         }
         
         // Update order size limits if provided
         if (params.maxOrderSize > 0) {
             if (!m_orderValidator->updateMinOrderSize(
                     params.exchangeId, params.tradingPair, params.minOrderSize)) {
                 LOG_ERROR("Failed to update minimum order size");
                 return false;
             }
         }
         
         return true;
     } catch (const std::exception& ex) {
         LOG_ERROR("Failed to update risk parameters: {}", ex.what());
         return false;
     }
 }
 
 RiskParameters PreTradeRiskFilter::getRiskParameters(const ExchangeId& exchangeId,
                                                    const TradingPair& tradingPair,
                                                    const StrategyId& strategyId) const {
     RiskParameters params;
     params.exchangeId = exchangeId;
     params.tradingPair = tradingPair;
     params.strategyId = strategyId;
     
     // Get position limits
     params.maxPositionSize = m_positionLimitChecker->getPositionLimit(
         tradingPair.getBaseAsset(), strategyId);
     
     // Get exposure limits
     const ExposureLimitConfig& exposureLimits = m_exposureCalculator->getExposureLimits();
     
     // Find the most specific limit that applies
     if (!strategyId.empty()) {
         auto it = exposureLimits.strategyExposureLimits.find(strategyId);
         if (it != exposureLimits.strategyExposureLimits.end()) {
             params.maxExposure = it->second;
         }
     }
     
     if (params.maxExposure <= 0 && !exchangeId.empty()) {
         auto it = exposureLimits.exchangeExposureLimits.find(exchangeId);
         if (it != exposureLimits.exchangeExposureLimits.end()) {
             params.maxExposure = it->second;
         }
     }
     
     if (params.maxExposure <= 0 && !tradingPair.getSymbol().empty()) {
         auto it = exposureLimits.assetExposureLimits.find(tradingPair.getBaseAsset());
         if (it != exposureLimits.assetExposureLimits.end()) {
             params.maxExposure = it->second;
         }
     }
     
     if (params.maxExposure <= 0) {
         params.maxExposure = exposureLimits.defaultAssetExposureLimit;
     }
     
     return params;
 }
 
 void PreTradeRiskFilter::setMode(RiskManagerMode mode) {
     m_activeMode = mode;
     LOG_INFO("Pre-trade risk filter mode changed to: {}", static_cast<int>(mode));
 }
 
 RiskManagerMode PreTradeRiskFilter::getMode() const {
     return m_activeMode;
 }
 
 ValidationResult PreTradeRiskFilter::performAllChecks(const TradingSignal& signal) {
     ValidationResult result;
     result.signalId = signal.id;
     result.approved = true;
     result.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
     
     // Check basic validation first
     if (!passesBasicValidation(signal, result.violations)) {
         result.approved = false;
         LOG_INFO("Signal {} rejected: failed basic validation", signal.id);
         return result;
     }
     
     // Check position limits
     if (!m_positionLimitChecker->checkPositionLimit(signal, result.violations)) {
         result.approved = false;
         LOG_INFO("Signal {} rejected: failed position limit check", signal.id);
     }
     
     // Check exposure limits
     if (!m_exposureCalculator->checkExposureLimit(signal, result.violations)) {
         result.approved = false;
         LOG_INFO("Signal {} rejected: failed exposure check", signal.id);
     }
     
     // Check order validation
     if (!m_orderValidator->validateOrder(signal, result.violations)) {
         result.approved = false;
         LOG_INFO("Signal {} rejected: failed order validation", signal.id);
     }
     
     // Check fat finger protection
     if (!m_fatFingerProtection->checkFatFinger(signal, result.violations)) {
         result.approved = false;
         LOG_INFO("Signal {} rejected: failed fat finger check", signal.id);
     }
     
     // If approved, adjust size if needed (e.g., due to risk constraints)
     if (result.approved) {
         result.adjustedSize = signal.targetSize;
         LOG_INFO("Signal {} approved", signal.id);
     }
     
     return result;
 }
 
 ValidationResult PreTradeRiskFilter::performAllChecks(const OrderRequest& orderRequest) {
     ValidationResult result;
     result.signalId = orderRequest.clientOrderId;
     result.approved = true;
     result.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
     
     // Check basic validation first
     if (!passesBasicValidation(orderRequest, result.violations)) {
         result.approved = false;
         LOG_INFO("Order {} rejected: failed basic validation", orderRequest.clientOrderId);
         return result;
     }
     
     // Check position limits
     if (!m_positionLimitChecker->checkPositionLimit(orderRequest, result.violations)) {
         result.approved = false;
         LOG_INFO("Order {} rejected: failed position limit check", orderRequest.clientOrderId);
     }
     
     // Check exposure limits
     if (!m_exposureCalculator->checkExposureLimit(orderRequest, result.violations)) {
         result.approved = false;
         LOG_INFO("Order {} rejected: failed exposure check", orderRequest.clientOrderId);
     }
     
     // Check order validation
     if (!m_orderValidator->validateOrder(orderRequest, result.violations)) {
         result.approved = false;
         LOG_INFO("Order {} rejected: failed order validation", orderRequest.clientOrderId);
     }
     
     // Check fat finger protection
     if (!m_fatFingerProtection->checkFatFinger(orderRequest, result.violations)) {
         result.approved = false;
         LOG_INFO("Order {} rejected: failed fat finger check", orderRequest.clientOrderId);
     }
     
     // If approved, adjust size if needed (e.g., due to risk constraints)
     if (result.approved) {
         result.adjustedSize = orderRequest.quantity;
         LOG_INFO("Order {} approved", orderRequest.clientOrderId);
     }
     
     return result;
 }
 bool PreTradeRiskFilter::passesBasicValidation(const TradingSignal& signal, 
                                             std::vector<RiskViolation>& violations) {
    // Check for missing required fields
    if (signal.id.empty()) {
        RiskViolation violation;
        violation.violationType = "MISSING_REQUIRED_FIELD";
        violation.description = "Signal ID is missing";
        violations.push_back(violation);
        return false;
    }
    
    if (signal.tradingPair.getSymbol().empty()) {
        RiskViolation violation;
        violation.violationType = "MISSING_REQUIRED_FIELD";
        violation.description = "Trading pair is missing";
        violations.push_back(violation);
        return false;
    }
    
    if (signal.exchangeId.empty()) {
        RiskViolation violation;
        violation.violationType = "MISSING_REQUIRED_FIELD";
        violation.description = "Exchange ID is missing";
        violations.push_back(violation);
        return false;
    }
    
    // Check for invalid values
    if (signal.targetSize <= 0) {
        RiskViolation violation;
        violation.violationType = "INVALID_FIELD_VALUE";
        violation.description = "Target size must be positive";
        violation.limitValue = 0;
        violation.actualValue = signal.targetSize;
        violations.push_back(violation);
        return false;
    }
    
    // Check for valid direction
    if (signal.direction != Direction::BUY && signal.direction != Direction::SELL) {
        RiskViolation violation;
        violation.violationType = "INVALID_FIELD_VALUE";
        violation.description = "Invalid direction value";
        violations.push_back(violation);
        return false;
    }
    
    // All basic validations passed
    return true;
}

bool PreTradeRiskFilter::passesBasicValidation(const OrderRequest& orderRequest, 
                                             std::vector<RiskViolation>& violations) {
    // Check for missing required fields
    if (orderRequest.clientOrderId.empty()) {
        RiskViolation violation;
        violation.violationType = "MISSING_REQUIRED_FIELD";
        violation.description = "Client order ID is missing";
        violations.push_back(violation);
        return false;
    }
    
    if (orderRequest.tradingPair.getSymbol().empty()) {
        RiskViolation violation;
        violation.violationType = "MISSING_REQUIRED_FIELD";
        violation.description = "Trading pair is missing";
        violations.push_back(violation);
        return false;
    }
    
    if (orderRequest.exchangeId.empty()) {
        RiskViolation violation;
        violation.violationType = "MISSING_REQUIRED_FIELD";
        violation.description = "Exchange ID is missing";
        violations.push_back(violation);
        return false;
    }
    
    // Check for invalid values
    if (orderRequest.quantity <= 0) {
        RiskViolation violation;
        violation.violationType = "INVALID_FIELD_VALUE";
        violation.description = "Order quantity must be positive";
        violation.limitValue = 0;
        violation.actualValue = orderRequest.quantity;
        violations.push_back(violation);
        return false;
    }
    
    // Check price for limit orders
    if (orderRequest.type == OrderType::LIMIT && orderRequest.price <= 0) {
        RiskViolation violation;
        violation.violationType = "INVALID_FIELD_VALUE";
        violation.description = "Limit price must be positive";
        violation.limitValue = 0;
        violation.actualValue = orderRequest.price;
        violations.push_back(violation);
        return false;
    }
    
    // Check for valid side
    if (orderRequest.side != OrderSide::BUY && orderRequest.side != OrderSide::SELL) {
        RiskViolation violation;
        violation.violationType = "INVALID_FIELD_VALUE";
        violation.description = "Invalid order side value";
        violations.push_back(violation);
        return false;
    }
    
    // All basic validations passed
    return true;
}

} // namespace risk
} // namespace trading