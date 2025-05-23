/**
 * Implementation of the exposure calculator
 *
 * File: exposure_calculator.cpp
 * Created: 2025-04-29
 */

 #include "risk/pretrade/exposure_calculator.hpp"
 #include "common/logger.hpp"
 
 #include <algorithm>
 #include <cmath>
 
 namespace trading {
 namespace risk {
 
 ExposureCalculator::ExposureCalculator(const RiskConfig& config,
                                       std::shared_ptr<PositionTracker> positionTracker,
                                       std::shared_ptr<DataInterface> dataInterface)
     : m_config(config)
     , m_positionTracker(positionTracker)
     , m_dataInterface(dataInterface)
     , m_totalExposureCache(0.0)
     , m_exposureCacheValid(false)
 {
     // Initialize with empty maps and zero exposure
 }
 
 bool ExposureCalculator::initialize() {
     try {
         // Load exposure limits from config
         m_exposureLimits = m_config.getExposureLimits();
         
         // Initialize with current market data
         MarketData initialData = m_dataInterface->getMarketData();
         updateMarketData(initialData);
         
         LOG_INFO("Exposure calculator initialized successfully");
         return true;
     } catch (const std::exception& ex) {
         LOG_ERROR("Failed to initialize exposure calculator: {}", ex.what());
         return false;
     }
 }
 
 bool ExposureCalculator::checkExposureLimit(const TradingSignal& signal,
                                            std::vector<RiskViolation>& violations) {
     // Get trading pair details
     TradingPair tradingPair = signal.tradingPair;
     std::string asset = tradingPair.getBaseAsset();
     std::string strategyId = signal.strategyId;
     ExchangeId exchangeId; = signal.exchangeId;
     
     // Calculate notional value of this trade
     double tradeNotional = calculateNotionalValue(tradingPair, signal.targetSize);
     
     // Apply volatility adjustment to get risk-adjusted exposure
     double volatility = getVolatility(tradingPair.getSymbol());
     double RISK_FACTOR = m_exposureLimits.riskFactor;
     double riskAdjustedExposure = tradeNotional * (1 + volatility * RISK_FACTOR);
     
     // Get current exposures (without this trade)
     double currentTotalExposure = calculateTotalExposure();
     double currentAssetExposure = calculateAssetExposure(asset);
     double currentStrategyExposure = calculateStrategyExposure(strategyId);
     double currentExchangeExposure = calculateExchangeExposure(exchangeId);
     
     // Get limits from config
     double totalExposureLimit = m_exposureLimits.totalExposureLimit;
     
     // Find asset-specific limit or use default
     double assetExposureLimit = m_exposureLimits.defaultAssetExposureLimit;
     auto assetLimitIt = m_exposureLimits.assetExposureLimits.find(asset);
     if (assetLimitIt != m_exposureLimits.assetExposureLimits.end()) {
         assetExposureLimit = assetLimitIt->second;
     }
     
     // Find strategy-specific limit or use default
     double strategyExposureLimit = m_exposureLimits.defaultStrategyExposureLimit;
     auto strategyLimitIt = m_exposureLimits.strategyExposureLimits.find(strategyId);
     if (strategyLimitIt != m_exposureLimits.strategyExposureLimits.end()) {
         strategyExposureLimit = strategyLimitIt->second;
     }
     
     // Find exchange-specific limit or use default
     double exchangeExposureLimit = m_exposureLimits.defaultExchangeExposureLimit;
     auto exchangeLimitIt = m_exposureLimits.exchangeExposureLimits.find(exchangeId.toString());
     if (exchangeLimitIt != m_exposureLimits.exchangeExposureLimits.end()) {
         exchangeExposureLimit = exchangeLimitIt->second;
     }
     
     // Check if adding this trade would exceed any limits
     bool withinLimits = true;
     
     // Check total exposure limit
     if (currentTotalExposure + riskAdjustedExposure > totalExposureLimit) {
         RiskViolation violation;
         violation.violationType = "TOTAL_EXPOSURE_LIMIT_EXCEEDED";
         violation.description = "Total exposure limit would be exceeded";
         violation.limitValue = totalExposureLimit;
         violation.actualValue = currentTotalExposure + riskAdjustedExposure;
         violations.push_back(violation);
         withinLimits = false;
     }
     
     // Check asset exposure limit
     if (currentAssetExposure + riskAdjustedExposure > assetExposureLimit) {
         RiskViolation violation;
         violation.violationType = "ASSET_EXPOSURE_LIMIT_EXCEEDED";
         violation.description = "Asset exposure limit would be exceeded for " + asset;
         violation.limitValue = assetExposureLimit;
         violation.actualValue = currentAssetExposure + riskAdjustedExposure;
         violations.push_back(violation);
         withinLimits = false;
     }
     
     // Check strategy exposure limit
     if (currentStrategyExposure + riskAdjustedExposure > strategyExposureLimit) {
         RiskViolation violation;
         violation.violationType = "STRATEGY_EXPOSURE_LIMIT_EXCEEDED";
         violation.description = "Strategy exposure limit would be exceeded for " + strategyId;
         violation.limitValue = strategyExposureLimit;
         violation.actualValue = currentStrategyExposure + riskAdjustedExposure;
         violations.push_back(violation);
         withinLimits = false;
     }
     
     // Check exchange exposure limit
     if (currentExchangeExposure + riskAdjustedExposure > exchangeExposureLimit) {
         RiskViolation violation;
         violation.violationType = "EXCHANGE_EXPOSURE_LIMIT_EXCEEDED";
         violation.description = "Exchange exposure limit would be exceeded for " + exchangeId;
         violation.limitValue = exchangeExposureLimit;
         violation.actualValue = currentExchangeExposure + riskAdjustedExposure;
         violations.push_back(violation);
         withinLimits = false;
     }
     
     return withinLimits;
 }
 
 bool ExposureCalculator::checkExposureLimit(const OrderRequest& orderRequest,
                                            std::vector<RiskViolation>& violations) {
     // Convert OrderRequest to equivalent signal for checking
     TradingSignal equivalentSignal;
     equivalentSignal.tradingPair = orderRequest.tradingPair;
     equivalentSignal.direction = (orderRequest.side == OrderSide::BUY) ? 
                                 Direction::BUY : Direction::SELL;
     equivalentSignal.targetSize = orderRequest.quantity;
     equivalentSignal.strategyId = orderRequest.strategyId;
     equivalentSignal.exchangeId = orderRequest.exchangeId;
     
     return checkExposureLimit(equivalentSignal, violations);
 }
 
 void ExposureCalculator::updateMarketData(const MarketData& marketData) {
     std::lock_guard<std::mutex> lock(m_marketDataMutex);
     
     // Update prices and volatilities
     for (const auto& pair : marketData.prices) {
         m_prices[pair.first] = pair.second;
     }
     
     for (const auto& pair : marketData.volatilities) {
         m_volatilities[pair.first] = pair.second;
     }
     
     // Invalidate exposure cache since market data has changed
     invalidateCache();
     
     LOG_DEBUG("Updated market data: {} prices, {} volatilities", 
               marketData.prices.size(), marketData.volatilities.size());
 }
 
 double ExposureCalculator::calculateTotalExposure() const {
     // Check if we have a valid cached value
     {
         std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
         if (m_exposureCacheValid) {
             return m_totalExposureCache;
         }
     }
     
     // Get all positions from position tracker
     std::vector<Position> positions = m_positionTracker->getAllPositions();
     
     // Calculate total exposure across all positions
     double totalExposure = 0.0;
     for (const auto& position : positions) {
         totalExposure += calculatePositionExposure(position);
     }
     
     // Update cache
     {
         std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
         m_totalExposureCache = totalExposure;
         m_exposureCacheValid = true;
     }
     
     return totalExposure;
 }
 
 double ExposureCalculator::calculateAssetExposure(const std::string& assetId) const {
     // Check if we have a valid cached value
     {
         std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
         auto it = m_assetExposureCache.find(assetId);
         if (m_exposureCacheValid && it != m_assetExposureCache.end()) {
             return it->second;
         }
     }
     
     // Get positions for this asset
     std::vector<Position> positions = m_positionTracker->getPositionsForAsset(assetId);
     
     // Calculate exposure for this asset
     double assetExposure = 0.0;
     for (const auto& position : positions) {
         assetExposure += calculatePositionExposure(position);
     }
     
     // Update cache
     {
         std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
         m_assetExposureCache[assetId] = assetExposure;
     }
     
     return assetExposure;
 }
 
 double ExposureCalculator::calculateStrategyExposure(const StrategyId& strategyId) const {
     // Check if we have a valid cached value
     {
         std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
         auto it = m_strategyExposureCache.find(strategyId);
         if (m_exposureCacheValid && it != m_strategyExposureCache.end()) {
             return it->second;
         }
     }
     
     // Get positions for this strategy
     std::vector<Position> positions = m_positionTracker->getPositionsForStrategy(strategyId);
     
     // Calculate exposure for this strategy
     double strategyExposure = 0.0;
     for (const auto& position : positions) {
         strategyExposure += calculatePositionExposure(position);
     }
     
     // Update cache
     {
         std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
         m_strategyExposureCache[strategyId] = strategyExposure;
     }
     
     return strategyExposure;
 }
 
 double ExposureCalculator::calculateExchangeExposure(const ExchangeId& exchangeId) const {
     // Check if we have a valid cached value
     {
         std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
         auto it = m_exchangeExposureCache.find(exchangeId.toString());
         if (m_exposureCacheValid && it != m_exchangeExposureCache.end()) {
             return it->second;
         }
     }
     
     // Get positions for this exchange
     std::vector<Position> positions = m_positionTracker->getPositionsForExchange(exchangeId);
     
     // Calculate exposure for this exchange
     double exchangeExposure = 0.0;
     for (const auto& position : positions) {
         exchangeExposure += calculatePositionExposure(position);
     }
     
     // Update cache
     {
         std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
         m_exchangeExposureCache[exchangeId] = exchangeExposure;
     }
     
     return exchangeExposure;
 }
 
 double ExposureCalculator::calculatePositionExposure(const Position& position) const {
     double quantity = position.quantity;
     std::string symbol = position.tradingPair.getSymbol();
     
     // Get price and volatility for this symbol
     double price = getPrice(symbol);
     double volatility = getVolatility(symbol);
     
     // Calculate notional value
     double notional = std::abs(quantity) * price;
     
     // Apply volatility adjustment
     double RISK_FACTOR = m_exposureLimits.riskFactor;
     double riskAdjustedExposure = notional * (1 + volatility * RISK_FACTOR);
     
     return riskAdjustedExposure;
 }
 
 double ExposureCalculator::getPrice(const std::string& symbol) const {
     std::lock_guard<std::mutex> lock(m_marketDataMutex);
     
     auto it = m_prices.find(symbol);
     if (it != m_prices.end()) {
         return it->second;
     }
     
     // Price not found, try to get from data interface
     try {
         double price = m_dataInterface->getPrice(symbol);
         
         // Cache the price for future use
         m_prices[symbol] = price;
         
         return price;
     } catch (const std::exception& ex) {
         LOG_ERROR("Failed to get price for {}: {}", symbol, ex.what());
         return 1.0; // Default to 1.0 to avoid division by zero
     }
 }
 
 double ExposureCalculator::getVolatility(const std::string& symbol) const {
     std::lock_guard<std::mutex> lock(m_marketDataMutex);
     
     auto it = m_volatilities.find(symbol);
     if (it != m_volatilities.end()) {
         return it->second;
     }
     
     // Volatility not found, try to get from data interface
     try {
         double volatility = m_dataInterface->getVolatility(symbol);
         
         // Cache the volatility for future use
         m_volatilities[symbol] = volatility;
         
         return volatility;
     } catch (const std::exception& ex) {
         LOG_ERROR("Failed to get volatility for {}: {}", symbol, ex.what());
         return 0.2; // Default to 20% as a conservative estimate
     }
 }
 
 void ExposureCalculator::invalidateCache() {
     std::lock_guard<std::mutex> lock(m_exposureCacheMutex);
     m_exposureCacheValid = false;
     m_assetExposureCache.clear();
     m_strategyExposureCache.clear();
     m_exchangeExposureCache.clear();
 }
 
 bool ExposureCalculator::updateExposureLimits(const ExposureLimitConfig& exposureLimits) {
     // Update exposure limits
     m_exposureLimits = exposureLimits;
     
     LOG_INFO("Updated exposure limits: total={}, default asset={}, default strategy={}, default exchange={}",
              exposureLimits.totalExposureLimit,
              exposureLimits.defaultAssetExposureLimit,
              exposureLimits.defaultStrategyExposureLimit,
              exposureLimits.defaultExchangeExposureLimit);
     
     return true;
 }
 
 ExposureLimitConfig ExposureCalculator::getExposureLimits() const {
     return m_exposureLimits;
 }
 
 double ExposureCalculator::calculateNotionalValue(const TradingPair& tradingPair, double quantity) const {
     std::string symbol = tradingPair.getSymbol();
     double price = getPrice(symbol);
     return std::abs(quantity) * price;
 }
 
 } // namespace risk
}