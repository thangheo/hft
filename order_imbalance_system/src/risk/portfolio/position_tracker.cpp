/**
 * Implementation of the position tracking component
 *
 * File: position_tracker.cpp
 * Created: 2025-04-29
 */

 #include "portfolio/position_tracker.hpp"
 #include <algorithm>
 #include <iostream>
 #include <sstream>
 namespace trading {
 namespace risk {
 
 // Position class implementation
 Position::Position(const ExchangeId& exchangeId, const TradingPair& tradingPair, 
                    const StrategyId& strategyId, double quantity, double averageEntryPrice)
     : m_exchangeId(exchangeId)
     , m_tradingPair(tradingPair)
     , m_strategyId(strategyId)
     , m_quantity(quantity)
     , m_averageEntryPrice(averageEntryPrice)
     , m_currentPrice(averageEntryPrice)
     , m_unrealizedPnl(0.0)
     , m_realizedPnl(0.0)
     , m_lastUpdated(std::chrono::system_clock::now().time_since_epoch().count()) {
 }
 
 // Position getters
 const ExchangeId& Position::getExchangeId() const { return m_exchangeId; }
 const TradingPair& Position::getTradingPair() const { return m_tradingPair; }
 const StrategyId& Position::getStrategyId() const { return m_strategyId; }
 double Position::getQuantity() const { return m_quantity; }
 double Position::getAverageEntryPrice() const { return m_averageEntryPrice; }
 double Position::getCurrentPrice() const { return m_currentPrice; }
 double Position::getUnrealizedPnl() const { return m_unrealizedPnl; }
 double Position::getRealizedPnl() const { return m_realizedPnl; }
 Timestamp Position::getLastUpdated() const { return m_lastUpdated; }
 
 // Position setters
 void Position::setCurrentPrice(double price) { 
     m_currentPrice = price;
     // Recalculate unrealized PnL
     if (m_quantity != 0) {
         m_unrealizedPnl = m_quantity * (m_currentPrice - m_averageEntryPrice);
     }
 }
 void Position::setUnrealizedPnl(double pnl) { m_unrealizedPnl = pnl; }
 void Position::setRealizedPnl(double pnl) { m_realizedPnl = pnl; }
 void Position::setLastUpdated(Timestamp timestamp) { m_lastUpdated = timestamp; }
 
 // Position operations
 void Position::updatePosition(double quantity, double price) {
     if (quantity == 0) return;
     
     // Calculate the new average entry price
     // If reducing position, keep the same average price
     if ((m_quantity > 0 && quantity < 0) || (m_quantity < 0 && quantity > 0)) {
         // Closing or reducing position
         if (std::abs(quantity) >= std::abs(m_quantity)) {
             // Complete position close or reversal
             double closeQuantity = m_quantity;
             double remainingQuantity = quantity + m_quantity;
             
             // Calculate realized PnL for the closed portion
             m_realizedPnl += closeQuantity * (price - m_averageEntryPrice);
             
             // Reset or establish new position
             if (remainingQuantity != 0) {
                 m_quantity = remainingQuantity;
                 m_averageEntryPrice = price;
             } else {
                 m_quantity = 0;
                 // Keep average entry price for reference
             }
         } else {
             // Partial position reduction
             m_realizedPnl += quantity * (price - m_averageEntryPrice);
             m_quantity += quantity;
         }
     } else {
         // Adding to position or establishing new position in same direction
         double totalCost = (m_quantity * m_averageEntryPrice) + (quantity * price);
         m_quantity += quantity;
         if (m_quantity != 0) {
             m_averageEntryPrice = totalCost / m_quantity;
         }
     }
     
     // Update timestamp
     m_lastUpdated = std::chrono::system_clock::now().time_since_epoch().count();
     
     // Recalculate unrealized PnL
     if (m_quantity != 0) {
         m_unrealizedPnl = m_quantity * (m_currentPrice - m_averageEntryPrice);
     } else {
         m_unrealizedPnl = 0;
     }
 }
 
 double Position::getNotionalValue() const {
     return std::abs(m_quantity) * m_currentPrice;
 }
 
 // PositionTracker implementation
 PositionTracker::PositionTracker(const RiskConfig& config, std::shared_ptr<DataInterface> dataInterface)
     : m_dataInterface(dataInterface)
     , m_config(config) {
 }
 
 bool PositionTracker::initialize() {
     try {
         // Initialize price cache from data interface if available
         if (m_dataInterface) {
             std::vector<MarketData> marketData = m_dataInterface->getLatestMarketData();
             for (const auto& md : marketData) {
                 std::lock_guard<std::mutex> lock(m_marketDataMutex);
                 m_prices[md.tradingPair] = md.price;
             }
         }
         
         return true;
     } catch (const std::exception& e) {
         std::cerr << "Error initializing PositionTracker: " << e.what() << std::endl;
         return false;
     }
 }
 
 void PositionTracker::processExecutionReport(const ExecutionReport& executionReport) {
     try {
         const auto& exchangeId = executionReport.exchangeId;
         const auto& tradingPair = executionReport.tradingPair;
         
         // Calculate total fill quantity and average price
         double fillQuantity = 0.0;
         double totalValue = 0.0;
         
         for (const auto& fill : executionReport.fills) {
             if (executionReport.side == "buy") {
                 fillQuantity += fill.quantity;
             } else {
                 fillQuantity -= fill.quantity;
             }
             totalValue += fill.quantity * fill.price;
         }
         
         if (fillQuantity == 0) return;
         
         double avgFillPrice = totalValue / std::abs(fillQuantity);
         
         // Update or create position
         std::lock_guard<std::mutex> lock(m_positionsMutex);
         std::string key = getPositionKey(exchangeId, tradingPair);
         
         if (m_positions.find(key) != m_positions.end()) {
             // Update existing position
             Position& position = m_positions[key];
             position.updatePosition(fillQuantity, avgFillPrice);
             updatePositionValue(position);
             notifyPositionUpdate(position, "modified");
         } else {
             // Create new position
             Position newPosition(exchangeId, tradingPair, executionReport.strategyId, fillQuantity, avgFillPrice);
             updatePositionValue(newPosition);
             m_positions[key] = newPosition;
             notifyPositionUpdate(newPosition, "new");
         }
     } catch (const std::exception& e) {
         std::cerr << "Error processing execution report: " << e.what() << std::endl;
     }
 }
 
 void PositionTracker::updateMarketData(const MarketData& marketData) {
     try {
         // Update price cache
         {
             std::lock_guard<std::mutex> lock(m_marketDataMutex);
             m_prices[marketData.tradingPair] = marketData.price;
         }
         
         // Update relevant positions
         std::lock_guard<std::mutex> lock(m_positionsMutex);
         for (auto& [key, position] : m_positions) {
             if (position.getTradingPair() == marketData.tradingPair) {
                 position.setCurrentPrice(marketData.price);
                 updatePositionValue(position);
                 notifyPositionUpdate(position, "price_update");
             }
         }
     } catch (const std::exception& e) {
         std::cerr << "Error updating market data: " << e.what() << std::endl;
     }
 }
 
 std::vector<Position> PositionTracker::getAllPositions() const {
     std::vector<Position> positions;
     std::lock_guard<std::mutex> lock(m_positionsMutex);
     
     positions.reserve(m_positions.size());
     for (const auto& [key, position] : m_positions) {
         positions.push_back(position);
     }
     
     return positions;
 }
 
 std::vector<Position> PositionTracker::getExchangePositions(const ExchangeId& exchangeId) const {
     std::vector<Position> positions;
     std::lock_guard<std::mutex> lock(m_positionsMutex);
     
     for (const auto& [key, position] : m_positions) {
         if (position.getExchangeId() == exchangeId) {
             positions.push_back(position);
         }
     }
     
     return positions;
 }
 
 std::vector<Position> PositionTracker::getPositionsByTradingPair(const TradingPair& tradingPair) const {
     std::vector<Position> positions;
     std::lock_guard<std::mutex> lock(m_positionsMutex);
     
     for (const auto& [key, position] : m_positions) {
         if (position.getTradingPair() == tradingPair) {
             positions.push_back(position);
         }
     }
     
     return positions;
 }
 
 std::optional<Position> PositionTracker::getPosition(const ExchangeId& exchangeId, const TradingPair& tradingPair) const {
     std::lock_guard<std::mutex> lock(m_positionsMutex);
     std::string key = getPositionKey(exchangeId, tradingPair);
     
     auto it = m_positions.find(key);
     if (it != m_positions.end()) {
         return it->second;
     }
     
     return std::nullopt;
 }
 
 std::vector<Position> PositionTracker::getStrategyPositions(const StrategyId& strategyId) const {
     std::vector<Position> positions;
     std::lock_guard<std::mutex> lock(m_positionsMutex);
     
     for (const auto& [key, position] : m_positions) {
         if (position.getStrategyId() == strategyId) {
             positions.push_back(position);
         }
     }
     
     return positions;
 }
 
 bool PositionTracker::updatePosition(const ExchangeId& exchangeId, const TradingPair& tradingPair, 
                                      double quantity, std::optional<double> price) {
     try {
         std::lock_guard<std::mutex> lock(m_positionsMutex);
         std::string key = getPositionKey(exchangeId, tradingPair);
         
         double positionPrice = price.value_or(getCurrentPrice(tradingPair));
         if (positionPrice <= 0) {
             return false;
         }
         
         if (m_positions.find(key) != m_positions.end()) {
             // Position exists, just set the quantity directly
             Position& position = m_positions[key];
             
             // Create a reconciliation adjustment
             double adjustment = quantity - position.getQuantity();
             if (adjustment != 0) {
                 position.updatePosition(adjustment, positionPrice);
                 updatePositionValue(position);
                 notifyPositionUpdate(position, "reconciliation");
             }
         } else if (quantity != 0) {
             // Create new position with default strategy ID
             Position newPosition(exchangeId, tradingPair, "reconciliation", quantity, positionPrice);
             updatePositionValue(newPosition);
             m_positions[key] = newPosition;
             notifyPositionUpdate(newPosition, "new_reconciliation");
         }
         
         return true;
     } catch (const std::exception& e) {
         std::cerr << "Error updating position: " << e.what() << std::endl;
         return false;
     }
 }
 
 double PositionTracker::calculatePortfolioValue() const {
     double totalValue = 0.0;
     std::lock_guard<std::mutex> lock(m_positionsMutex);
     
     for (const auto& [key, position] : m_positions) {
         totalValue += position.getNotionalValue();
     }
     
     return totalValue;
 }
 
 double PositionTracker::calculateTotalExposure() const {
     double totalExposure = 0.0;
     std::lock_guard<std::mutex> lock(m_positionsMutex);
     
     for (const auto& [key, position] : m_positions) {
         // Absolute value of position to account for both long and short
         totalExposure += position.getNotionalValue();
     }
     
     return totalExposure;
 }
 
 void PositionTracker::registerPositionUpdateCallback(const PositionUpdateCallback& callback) {
     std::lock_guard<std::mutex> lock(m_callbackMutex);
     m_updateCallbacks.push_back(callback);
 }
 
 void PositionTracker::clearPositions() {
     std::lock_guard<std::mutex> lock(m_positionsMutex);
     m_positions.clear();
 }
 
 void PositionTracker::notifyPositionUpdate(const Position& position, const std::string& updateType) {
     std::lock_guard<std::mutex> lock(m_callbackMutex);
     for (const auto& callback : m_updateCallbacks) {
         callback(position, updateType);
     }
 }
 
 bool PositionTracker::shouldCreateNewPosition(const ExchangeId& exchangeId, const TradingPair& tradingPair, Direction direction) {
     std::string key = getPositionKey(exchangeId, tradingPair);
     
     auto it = m_positions.find(key);
     if (it == m_positions.end()) {
         return true;
     }
     
     const Position& position = it->second;
     
     // Create new position if existing position is in opposite direction
     return (direction == Direction::Buy && position.getQuantity() < 0) ||
            (direction == Direction::Sell && position.getQuantity() > 0);
 }
 
 void PositionTracker::updatePositionValue(Position& position) {
     double currentPrice = getCurrentPrice(position.getTradingPair());
     if (currentPrice > 0) {
         position.setCurrentPrice(currentPrice);
         position.setLastUpdated(std::chrono::system_clock::now().time_since_epoch().count());
     }
 }
 
 std::string PositionTracker::getPositionKey(const ExchangeId& exchangeId, const TradingPair& tradingPair) const {
     return exchangeId + ":" + tradingPair;
 }
 
 double PositionTracker::getCurrentPrice(const TradingPair& tradingPair) const {
     std::lock_guard<std::mutex> lock(m_marketDataMutex);
     
     auto it = m_prices.find(tradingPair);
     if (it != m_prices.end()) {
         return it->second;
     }
     
     // If price not found in cache, try to get from data interface
     if (m_dataInterface) {
         try {
             auto marketData = m_dataInterface->getMarketData(tradingPair);
             if (marketData) {
                 return marketData->price;
             }
         } catch (...) {
             // Fall through to return 0
         }
     }
     
     return 0.0; // Price not available
 }
 
 } // namespace risk
} // namespace trading