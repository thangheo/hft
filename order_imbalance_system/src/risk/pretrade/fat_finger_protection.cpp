/**
 * Implementation of the fat finger protection
 *
 * File: fat_finger_protection.cpp
 * Created: 2025-04-29
 */

 #include "risk/pretrade/fat_finger_protection.hpp"
 #include "common/logger.hpp"
 
 #include <algorithm>
 #include <numeric>
 #include <cmath>
 #include <stdexcept>
 
 namespace trading {
 namespace risk {
 
 FatFingerProtection::FatFingerProtection(const RiskConfig& config,
                                         std::shared_ptr<DataInterface> dataInterface)
     : m_config(config)
     , m_dataInterface(dataInterface)
 {
     // Initialize with empty order history
 }
 
 bool FatFingerProtection::initialize() {
     try {
         // Load fat finger protection configuration
         m_fatFingerConfig = m_config.getFatFingerConfig();
         
         LOG_INFO("Fat finger protection initialized successfully");
         return true;
     } catch (const std::exception& ex) {
         LOG_ERROR("Failed to initialize fat finger protection: {}", ex.what());
         return false;
     }
 }
 
 bool FatFingerProtection::checkFatFinger(const TradingSignal& signal,
                                         std::vector<RiskViolation>& violations) {
     // For a trading signal, we only check size since we don't have a price yet
     return checkSizeAnomaly(signal.tradingPair, signal.targetSize, violations);
 }
 
 bool FatFingerProtection::checkFatFinger(const OrderRequest& orderRequest,
                                         std::vector<RiskViolation>& violations) {
     // For a full order, check price deviation, size, and notional value
     bool priceOk = true;
     bool sizeOk = true;
     bool notionalOk = true;
     
     // For market orders, we can't check price deviation
     if (orderRequest.type == OrderType::LIMIT) {
         priceOk = checkPriceDeviation(orderRequest.tradingPair, orderRequest.price, violations);
     }
     
     // Check size anomaly
     sizeOk = checkSizeAnomaly(orderRequest.tradingPair, orderRequest.quantity, violations);
     
     // Check notional value
     double price = (orderRequest.type == OrderType::LIMIT) ? 
                   orderRequest.price : getCurrentMarketPrice(orderRequest.tradingPair);
     notionalOk = checkNotionalValue(orderRequest.tradingPair, price, orderRequest.quantity, violations);
     
     return priceOk && sizeOk && notionalOk;
 }
 
 void FatFingerProtection::recordOrder(const OrderRequest& order) {
     std::lock_guard<std::mutex> lock(m_historyMutex);
     
     std::string symbol = order.tradingPair.getSymbol();
     
     // Add order size to history
     m_orderSizeHistory[symbol].push_back(order.quantity);
     
     // Ensure we don't exceed maximum history size
     if (m_orderSizeHistory[symbol].size() > MAX_HISTORY_SIZE) {
         m_orderSizeHistory[symbol].pop_front();
     }
     
     // Recalculate typical size for this symbol
     recalculateTypicalSize(symbol);
     
     LOG_DEBUG("Recorded order for {}: size={}, history size={}", 
               symbol, order.quantity, m_orderSizeHistory[symbol].size());
 }
 
 bool FatFingerProtection::updateSettings(const FatFingerConfig& fatFingerConfig) {
     std::lock_guard<std::mutex> lock(m_configMutex);
     
     // Update fat finger protection settings
     m_fatFingerConfig = fatFingerConfig;
     
     LOG_INFO("Updated fat finger protection settings: price deviation={}, size anomaly={}, notional threshold={}",
              m_fatFingerConfig.priceDeviationThreshold,
              m_fatFingerConfig.sizeAnomalyThreshold,
              m_fatFingerConfig.notionalValueThreshold);
     
     return true;
 }
 
 bool FatFingerProtection::requiresAdditionalVerification(const TradingPair& tradingPair,
                                                        double price,
                                                        double quantity) const {
     double notionalValue = calculateNotionalValue(price, quantity);
     
     std::lock_guard<std::mutex> lock(m_configMutex);
     return notionalValue > m_fatFingerConfig.notionalValueThreshold;
 }
 
 double FatFingerProtection::getTypicalOrderSize(const TradingPair& tradingPair) const {
     std::lock_guard<std::mutex> lock(m_historyMutex);
     
     std::string symbol = tradingPair.getSymbol();
     
     // Check if we have a cached typical size for this symbol
     auto it = m_typicalOrderSizeCache.find(symbol);
     if (it != m_typicalOrderSizeCache.end()) {
         return it->second;
     }
     
     // Check if we have order history for this symbol
     auto historyIt = m_orderSizeHistory.find(symbol);
     if (historyIt == m_orderSizeHistory.end() || historyIt->second.empty()) {
         // No history, return a default value
         return m_fatFingerConfig.defaultTypicalSize;
     }
     
     // Calculate median order size (more robust than mean)
     std::vector<double> sizes(historyIt->second.begin(), historyIt->second.end());
     std::sort(sizes.begin(), sizes.end());
     double median;
     if (sizes.size() % 2 == 0) {
         median = (sizes[sizes.size() / 2 - 1] + sizes[sizes.size() / 2]) / 2.0;
     } else {
         median = sizes[sizes.size() / 2];
     }
     
     return median;
 }
 
 bool FatFingerProtection::checkPriceDeviation(const TradingPair& tradingPair,
                                             double price,
                                             std::vector<RiskViolation>& violations) {
     // Get current market price
     double marketPrice;
     try {
         marketPrice = getCurrentMarketPrice(tradingPair);
     } catch (const std::exception& ex) {
         LOG_ERROR("Failed to get market price for {}: {}", tradingPair.getSymbol(), ex.what());
         
         // If we can't get the market price, conservatively reject the order
         RiskViolation violation;
         violation.violationType = "PRICE_CHECK_FAILED";
         violation.description = "Could not verify price due to market data unavailability";
         violation.limitValue = 0;
         violation.actualValue = 0;
         violations.push_back(violation);
         return false;
     }
     
     // Calculate price deviation as a percentage
     double deviation = std::abs((price - marketPrice) / marketPrice);
     
     // Check if deviation exceeds the threshold
     std::lock_guard<std::mutex> lock(m_configMutex);
     double threshold = m_fatFingerConfig.priceDeviationThreshold;
     
     if (deviation > threshold) {
         RiskViolation violation;
         violation.violationType = "PRICE_DEVIATION_TOO_LARGE";
         violation.description = "Price deviates significantly from market price";
         violation.limitValue = threshold * 100; // Convert to percentage for reporting
         violation.actualValue = deviation * 100;
         violations.push_back(violation);
         return false;
     }
     
     return true;
 }
 
 bool FatFingerProtection::checkSizeAnomaly(const TradingPair& tradingPair,
                                          double quantity,
                                          std::vector<RiskViolation>& violations) {
     // Get typical order size for this trading pair
     double typicalSize = getTypicalOrderSize(tradingPair);
     
     // Calculate ratio of current order size to typical size
     double sizeRatio = quantity / typicalSize;
     
     // Check if ratio exceeds the threshold
     std::lock_guard<std::mutex> lock(m_configMutex);
     double threshold = m_fatFingerConfig.sizeAnomalyThreshold;
     
     if (sizeRatio > threshold) {
         RiskViolation violation;
         violation.violationType = "ORDER_SIZE_ANOMALY";
         violation.description = "Order size significantly larger than typical";
         violation.limitValue = threshold * typicalSize;
         violation.actualValue = quantity;
         violations.push_back(violation);
         return false;
     }
     
     return true;
 }
 
 bool FatFingerProtection::checkNotionalValue(const TradingPair& tradingPair,
                                            double price,
                                            double quantity,
                                            std::vector<RiskViolation>& violations) {
     // Calculate notional value (price * quantity)
     double notionalValue = calculateNotionalValue(price, quantity);
     
     // Check if notional value exceeds the threshold
     std::lock_guard<std::mutex> lock(m_configMutex);
     double threshold = m_fatFingerConfig.notionalValueThreshold;
     
     if (notionalValue > threshold) {
         // For high notional value orders, we may either reject or require additional verification
         if (m_fatFingerConfig.rejectHighNotionalOrders) {
             RiskViolation violation;
             violation.violationType = "NOTIONAL_VALUE_TOO_LARGE";
             violation.description = "Order notional value exceeds maximum allowed";
             violation.limitValue = threshold;
             violation.actualValue = notionalValue;
             violations.push_back(violation);
             return false;
         } else {
             // Mark for additional verification but don't reject outright
             RiskViolation violation;
             violation.violationType = "REQUIRES_ADDITIONAL_VERIFICATION";
             violation.description = "Order requires additional verification due to high notional value";
             violation.limitValue = threshold;
             violation.actualValue = notionalValue;
             violations.push_back(violation);
             // Still return true since we're not rejecting
         }
     }
     
     return true;
 }
 
 void FatFingerProtection::recalculateTypicalSize(const std::string& symbol) {
     auto it = m_orderSizeHistory.find(symbol);
     if (it == m_orderSizeHistory.end() || it->second.empty()) {
         return;
     }
     
     // Calculate median order size
     std::vector<double> sizes(it->second.begin(), it->second.end());
     std::sort(sizes.begin(), sizes.end());
     double median;
     if (sizes.size() % 2 == 0) {
         median = (sizes[sizes.size() / 2 - 1] + sizes[sizes.size() / 2]) / 2.0;
     } else {
         median = sizes[sizes.size() / 2];
     }
     
     // Update cache
     m_typicalOrderSizeCache[symbol] = median;
 }
 
 double FatFingerProtection::getCurrentMarketPrice(const TradingPair& tradingPair) const {
     return m_dataInterface->getPrice(tradingPair.getSymbol());
 }
 
 double FatFingerProtection::calculateNotionalValue(double price, double quantity) const {
     return price * std::abs(quantity);
 }
 
 } // namespace risk
}