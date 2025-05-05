/**
 * Implementation of the position limit checker
 *
 * File: position_limit_checker.cpp
 * Created: 2025-04-29
 */

 #include "risk/pretrade/position_limit_checker.hpp"
 #include "common/logger.hpp"
 
 #include <algorithm>
 #include <cmath>
 
 namespace trading {
 namespace risk {
 
 PositionLimitChecker::PositionLimitChecker(const RiskConfig& config, 
                                           std::shared_ptr<PositionTracker> positionTracker)
     : m_config(config)
     , m_positionTracker(positionTracker)
     , m_defaultLimit(1000.0) // Default limit if not specified
 {
     // Initialize empty maps for limits
 }
 
 bool PositionLimitChecker::initialize() {
     try {
         loadLimitsFromConfig();
         LOG_INFO("Position limit checker initialized successfully");
         return true;
     } catch (const std::exception& ex) {
         LOG_ERROR("Failed to initialize position limit checker: {}", ex.what());
         return false;
     }
 }
 
 void PositionLimitChecker::loadLimitsFromConfig() {
     std::lock_guard<std::mutex> lock(m_limitsLock);
     
     // Load position limits from config
     m_positionLimits = m_config.getPositionLimits();
     m_defaultLimit = m_positionLimits.defaultLimit;
     
     // Load asset-specific limits
     for (const auto& asset : m_positionLimits.assetLimits) {
         m_assetLimits[asset.first] = asset.second;
     }
     
     // Load strategy-specific limits
     for (const auto& strategy : m_positionLimits.strategyLimits) {
         m_strategyLimits[strategy.first] = strategy.second;
     }
     
     // Load exchange-specific limits
     for (const auto& exchange : m_positionLimits.exchangeLimits) {
         m_exchangeLimits[exchange.first] = exchange.second;
     }
     
     LOG_INFO("Loaded position limits: {} assets, {} strategies, {} exchanges",
              m_assetLimits.size(), m_strategyLimits.size(), m_exchangeLimits.size());
 }
 
 bool PositionLimitChecker::checkPositionLimit(const TradingSignal& signal, 
                                              std::vector<RiskViolation>& violations) {
     // Extract trading pair from the signal
     TradingPair tradingPair = signal.tradingPair;
     std::string asset = tradingPair.getBaseAsset();
     std::string strategyId = signal.strategyId;
     std::string exchangeId = signal.exchangeId;
     
     // Calculate new position after this trade
     double newPosition = calculateNewPosition(
         tradingPair, 
         signal.direction, 
         signal.targetSize
     );
     
     // Get applicable limits
     double assetLimit = getPositionLimit(asset, strategyId);
     double strategyLimit = 0.0;
     double exchangeLimit = 0.0;
     
     {
         std::lock_guard<std::mutex> lock(m_limitsLock);
         strategyLimit = m_strategyLimits.count(strategyId) ? 
                         m_strategyLimits.at(strategyId) : m_defaultLimit;
         exchangeLimit = m_exchangeLimits.count(exchangeId) ? 
                        m_exchangeLimits.at(exchangeId) : m_defaultLimit;
     }
     
     bool withinLimits = true;
     
     // Check asset-specific limit
     if (std::abs(newPosition) > assetLimit) {
         RiskViolation violation;
         violation.violationType = "POSITION_LIMIT_EXCEEDED";
         violation.description = "Asset position limit exceeded for " + asset;
         violation.limitValue = assetLimit;
         violation.actualValue = std::abs(newPosition);
         violations.push_back(violation);
         withinLimits = false;
     }
     
     // Check strategy limit (if applicable)
     double totalStrategyPosition = m_positionTracker->getTotalPositionForStrategy(strategyId);
     totalStrategyPosition += signal.targetSize; // Add this signal
     
     if (std::abs(totalStrategyPosition) > strategyLimit) {
         RiskViolation violation;
         violation.violationType = "STRATEGY_POSITION_LIMIT_EXCEEDED";
         violation.description = "Strategy position limit exceeded for " + strategyId;
         violation.limitValue = strategyLimit;
         violation.actualValue = std::abs(totalStrategyPosition);
         violations.push_back(violation);
         withinLimits = false;
     }
     
     // Check exchange limit (if applicable)
     double totalExchangePosition = m_positionTracker->getTotalPositionForExchange(exchangeId);
     totalExchangePosition += signal.targetSize; // Add this signal
     
     if (std::abs(totalExchangePosition) > exchangeLimit) {
         RiskViolation violation;
         violation.violationType = "EXCHANGE_POSITION_LIMIT_EXCEEDED";
         violation.description = "Exchange position limit exceeded for " + exchangeId;
         violation.limitValue = exchangeLimit;
         violation.actualValue = std::abs(totalExchangePosition);
         violations.push_back(violation);
         withinLimits = false;
     }
     
     return withinLimits;
 }
 
 bool PositionLimitChecker::checkPositionLimit(const OrderRequest& orderRequest, 
                                              std::vector<RiskViolation>& violations) {
     // Convert OrderRequest to equivalent checks
     TradingSignal equivalentSignal;
     equivalentSignal.tradingPair = orderRequest.tradingPair;
     equivalentSignal.direction = (orderRequest.side == OrderSide::BUY) ? 
                                 Direction::BUY : Direction::SELL;
     equivalentSignal.targetSize = orderRequest.quantity;
     equivalentSignal.strategyId = orderRequest.strategyId;
     equivalentSignal.exchangeId = orderRequest.exchangeId;
     
     return checkPositionLimit(equivalentSignal, violations);
 }
 
 double PositionLimitChecker::calculateNewPosition(const TradingPair& tradingPair, 
                                                  Direction direction, 
                                                  double quantity) const {
     std::string baseAsset = tradingPair.getBaseAsset();
     
     // Get current position
     double currentPosition = m_positionTracker->getPosition(baseAsset);
     
     // Calculate new position based on direction
     double change = (direction == Direction::BUY) ? quantity : -quantity;
     double newPosition = currentPosition + change;
     
     return newPosition;
 }
 
 bool PositionLimitChecker::updatePositionLimit(const std::string& assetId, 
                                               const std::string& strategyId, 
                                               double newLimit) {
     if (newLimit <= 0) {
         LOG_ERROR("Invalid position limit value: {}", newLimit);
         return false;
     }
     
     std::lock_guard<std::mutex> lock(m_limitsLock);
     
     if (!strategyId.empty()) {
         // Update strategy limit
         m_strategyLimits[strategyId] = newLimit;
         LOG_INFO("Updated position limit for strategy {}: {}", strategyId, newLimit);
     } else if (!assetId.empty()) {
         // Update asset limit
         m_assetLimits[assetId] = newLimit;
         LOG_INFO("Updated position limit for asset {}: {}", assetId, newLimit);
     } else {
         // Update default limit
         m_defaultLimit = newLimit;
         LOG_INFO("Updated default position limit: {}", newLimit);
     }
     
     return true;
 }
 
 double PositionLimitChecker::getPositionLimit(const std::string& assetId, 
                                              const std::string& strategyId) const {
     std::lock_guard<std::mutex> lock(m_limitsLock);
     
     // First check for asset+strategy specific limit
     std::string combinedKey = assetId + ":" + strategyId;
     auto it = m_assetLimits.find(combinedKey);
     if (it != m_assetLimits.end()) {
         return it->second;
     }
     
     // Then check for asset-specific limit
     it = m_assetLimits.find(assetId);
     if (it != m_assetLimits.end()) {
         return it->second;
     }
     
     // Then check for strategy-specific limit
     auto strat_it = m_strategyLimits.find(strategyId);
     if (strat_it != m_strategyLimits.end()) {
         return strat_it->second;
     }
     
     // Fall back to default limit
     return m_defaultLimit;
 }
 
 } // namespace risk
}