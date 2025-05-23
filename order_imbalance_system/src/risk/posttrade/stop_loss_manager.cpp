#include "risk/posttrade/stop_loss_manager.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace trading {
namespace risk {

StopLossManager::StopLossManager(const RiskConfig& config, 
                                std::shared_ptr<PositionTracker> positionTracker,
                                std::shared_ptr<DataInterface> dataInterface,
                                std::shared_ptr<ExecutionInterface> executionInterface)
    : m_positionTracker(positionTracker),
      m_dataInterface(dataInterface),
      m_executionInterface(executionInterface),
      m_config(config),
      m_stopLossConfig(config.stopLossConfig) {
}

bool StopLossManager::initialize() {
    // Load initial positions
    auto positions = m_positionTracker->getAllPositions();
    
    // Initialize stop losses based on default configuration
    for (const auto& position : positions) {
        const core::ExchangeId& exchangeId = position.exchangeId;
        const std::string& tradingPair = position.tradingPair;
        
        // Skip zero positions
        if (std::abs(position.quantity) < 1e-8) {
            continue;
        }
        
        // Check if we should auto-create stop losses based on configuration
        if (m_stopLossConfig.enableDefaultStopLoss) {
            double stopPrice = 0.0;
            
            // Calculate stop price based on position direction
            if (position.quantity > 0) { // Long position
                stopPrice = position.averageEntryPrice * (1.0 - m_stopLossConfig.defaultStopLossPercent);
            } else { // Short position
                stopPrice = position.averageEntryPrice * (1.0 + m_stopLossConfig.defaultStopLossPercent);
            }
            
            // Set the stop loss
            setStopLoss(exchangeId, tradingPair, stopPrice, 
                      m_stopLossConfig.useTrailingStopByDefault,
                      m_stopLossConfig.defaultTrailingDistance);
        }
    }
    
    return true;
}

void StopLossManager::processExecutionReport(const ExecutionReport& executionReport) {
    // When new trades occur, we may need to set or adjust stop losses
    const core::ExchangeId& exchangeId = executionReport.exchangeId;
    const std::string& tradingPair = executionReport.tradingPair;
    
    // Check if position has been closed
    if (std::abs(executionReport.remainingQuantity) < 1e-8) {
        // Position closed, remove any stop loss
        removeStopLoss(exchangeId, tradingPair);
        return;
    }
    
    // Fetch the current position after this execution
    auto position = m_positionTracker->getPosition(exchangeId, tradingPair);
    if (!position) {
        // No position found, nothing to do
        return;
    }
    
    // Get current stop loss if it exists
    auto stopLossOpt = getStopLoss(exchangeId, tradingPair);
    
    // If no stop loss exists and auto-create is enabled, create one
    if (!stopLossOpt && m_stopLossConfig.enableDefaultStopLoss) {
        double stopPrice = 0.0;
        
        // Calculate stop price based on position direction
        if (position->quantity > 0) { // Long position
            stopPrice = position->averageEntryPrice * (1.0 - m_stopLossConfig.defaultStopLossPercent);
        } else { // Short position
            stopPrice = position->averageEntryPrice * (1.0 + m_stopLossConfig.defaultStopLossPercent);
        }
        
        // Set the stop loss
        setStopLoss(exchangeId, tradingPair, stopPrice, 
                  m_stopLossConfig.useTrailingStopByDefault,
                  m_stopLossConfig.defaultTrailingDistance);
    }
    // If stop loss exists and position direction changed, adjust stop loss
    else if (stopLossOpt && 
            ((position->quantity > 0 && stopLossOpt->stopPrice > position->averageEntryPrice) ||
             (position->quantity < 0 && stopLossOpt->stopPrice < position->averageEntryPrice))) {
        
        double stopPrice = 0.0;
        
        // Calculate stop price based on position direction
        if (position->quantity > 0) { // Long position
            stopPrice = position->averageEntryPrice * (1.0 - m_stopLossConfig.defaultStopLossPercent);
        } else { // Short position
            stopPrice = position->averageEntryPrice * (1.0 + m_stopLossConfig.defaultStopLossPercent);
        }
        
        // Update the stop loss
        setStopLoss(exchangeId, tradingPair, stopPrice, 
                  stopLossOpt->isTrailing,
                  stopLossOpt->trailingDistance);
    }
}

void StopLossManager::updateMarketData(const MarketData& marketData) {
    // Update our last price cache
    {
        std::lock_guard<std::mutex> lock(m_marketDataMutex);
        m_lastPrices[marketData.symbol] = marketData.lastPrice;
    }
    
    // Update trailing stops based on new prices
    updateTrailingStops(marketData.symbol, marketData.lastPrice);
}

bool StopLossManager::setStopLoss(const ExchangeId& exchangeId, 
                                const TradingPair& tradingPair, 
                                double stopPrice, 
                                bool isTrailing,
                                double trailingDistance) {
    // Validate inputs
    if (stopPrice <= 0 || (isTrailing && trailingDistance <= 0)) {
        return false;
    }
    
    // Get current position
    auto position = m_positionTracker->getPosition(exchangeId, tradingPair);
    if (!position || std::abs(position->quantity) < 1e-8) {
        // Cannot set stop loss for non-existent or zero position
        return false;
    }
    
    // Get current price
    double currentPrice = 0.0;
    {
        std::lock_guard<std::mutex> lock(m_marketDataMutex);
        auto it = m_lastPrices.find(tradingPair);
        if (it != m_lastPrices.end()) {
            currentPrice = it->second;
        } else {
            // Try to fetch current price from data interface
            auto priceOpt = m_dataInterface->getLastPrice(exchangeId, tradingPair);
            if (priceOpt) {
                currentPrice = *priceOpt;
                m_lastPrices[tradingPair] = currentPrice;
            } else {
                // Cannot proceed without price information
                return false;
            }
        }
    }
    
    // Create stop loss info
    StopLossInfo stopLossInfo;
    stopLossInfo.stopPrice = stopPrice;
    stopLossInfo.isTrailing = isTrailing;
    stopLossInfo.trailingDistance = trailingDistance;
    stopLossInfo.active = true;
    stopLossInfo.lastUpdated = std::chrono::system_clock::now();
    
    // For trailing stops, set highest/lowest price
    if (isTrailing) {
        if (position->quantity > 0) { // Long position
            stopLossInfo.highestPrice = currentPrice;
            stopLossInfo.lowestPrice = 0;
        } else { // Short position
            stopLossInfo.lowestPrice = currentPrice;
            stopLossInfo.highestPrice = 0;
        }
    }
    
    // Store the stop loss
    {
        std::lock_guard<std::mutex> lock(m_stopLossMutex);
        m_stopLosses[{exchangeId, tradingPair}] = stopLossInfo;
    }
    
    return true;
}

bool StopLossManager::removeStopLoss(const ExchangeId& exchangeId, const TradingPair& tradingPair) {
    std::lock_guard<std::mutex> lock(m_stopLossMutex);
    return m_stopLosses.erase({exchangeId, tradingPair}) > 0;
}

std::optional<StopLossInfo> StopLossManager::getStopLoss(const ExchangeId& exchangeId, const TradingPair& tradingPair) const {
    std::lock_guard<std::mutex> lock(m_stopLossMutex);
    auto it = m_stopLosses.find({exchangeId, tradingPair});
    if (it != m_stopLosses.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<StopLossEvent> StopLossManager::checkStopLosses() {
    std::vector<StopLossEvent> triggeredEvents;
    
    // Make a copy of stop losses to avoid holding lock during processing
    std::map<std::pair<std::string, std::string>, StopLossInfo> stopLossesCopy;
    {
        std::lock_guard<std::mutex> lock(m_stopLossMutex);
        stopLossesCopy = m_stopLosses;
    }
    
    // Check each stop loss against current prices
    for (const auto& [key, stopLoss] : stopLossesCopy) {
        const auto& [exchangeId, tradingPair] = key;
        
        // Skip inactive stop losses
        if (!stopLoss.active) {
            continue;
        }
        
        // Get current price
        double currentPrice = 0.0;
        {
            std::lock_guard<std::mutex> lock(m_marketDataMutex);
            auto it = m_lastPrices.find(tradingPair);
            if (it != m_lastPrices.end()) {
                currentPrice = it->second;
            } else {
                // Try to fetch current price from data interface
                auto priceOpt = m_dataInterface->getLastPrice(exchangeId, tradingPair);
                if (priceOpt) {
                    currentPrice = *priceOpt;
                    m_lastPrices[tradingPair] = currentPrice;
                } else {
                    // Skip if no price information
                    continue;
                }
            }
        }
        
        // Get current position
        auto position = m_positionTracker->getPosition(exchangeId, tradingPair);
        if (!position || std::abs(position->quantity) < 1e-8) {
            // Position doesn't exist or is zero, remove stop loss
            removeStopLoss(exchangeId, tradingPair);
            continue;
        }
        
        // Check if stop loss is triggered
        if (isStopLossTriggered(stopLoss, currentPrice, position->quantity)) {
            // Create stop loss event
            double lossPercentage = std::abs((currentPrice - position->averageEntryPrice) / position->averageEntryPrice);
            auto event = createStopLossEvent(exchangeId, tradingPair, position->quantity, 
                                           position->averageEntryPrice, currentPrice, lossPercentage);
            
            // Add to triggered events
            triggeredEvents.push_back(event);
            
            // Execute liquidation if auto-liquidation is enabled
            if (m_stopLossConfig.autoLiquidateOnTrigger) {
                liquidatePosition(exchangeId, tradingPair);
            }
            
            // Notify callbacks
            for (const auto& callback : m_stopLossCallbacks) {
                callback(event);
            }
            
            // Deactivate the stop loss
            std::lock_guard<std::mutex> lock(m_stopLossMutex);
            auto it = m_stopLosses.find({exchangeId, tradingPair});
            if (it != m_stopLosses.end()) {
                it->second.active = false;
            }
        }
    }
    
    return triggeredEvents;
}

bool StopLossManager::liquidatePosition(const ExchangeId& exchangeId, const TradingPair& tradingPair) {
    // Get current position
    auto position = m_positionTracker->getPosition(exchangeId, tradingPair);
    if (!position || std::abs(position->quantity) < 1e-8) {
        return false;
    }
    
    // Create a market order to liquidate the position
    Order liquidationOrder;
    liquidationOrder.exchangeId = exchangeId;
    liquidationOrder.tradingPair = tradingPair;
    liquidationOrder.orderType = "MARKET";
    liquidationOrder.side = position->quantity > 0 ? "SELL" : "BUY";
    liquidationOrder.quantity = std::abs(position->quantity);
    liquidationOrder.timeInForce = "IOC";
    liquidationOrder.clientOrderId = "STOPLOSS_" + exchangeId + "_" + tradingPair + "_" + 
                                   std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    // Submit the order via execution interface
    return m_executionInterface->submitOrder(liquidationOrder);
}

bool StopLossManager::updateConfiguration(const StopLossConfig& stopLossConfig) {
    m_stopLossConfig = stopLossConfig;
    return true;
}

StopLossConfig StopLossManager::getConfiguration() const {
    return m_stopLossConfig;
}

void StopLossManager::registerStopLossCallback(const std::function<void(const StopLossEvent&)>& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_stopLossCallbacks.push_back(callback);
}

void StopLossManager::updateTrailingStops(const std::string& symbol, double price) {
    std::lock_guard<std::mutex> lock(m_stopLossMutex);
    
    // Update all trailing stops for this symbol
    for (auto& [key, stopLoss] : m_stopLosses) {
        const auto& [exchangeId, tradingPair] = key;
        
        // Skip if not for this symbol or not a trailing stop
        if (tradingPair != symbol || !stopLoss.isTrailing || !stopLoss.active) {
            continue;
        }
        
        // Get position direction
        auto position = m_positionTracker->getPosition(exchangeId, tradingPair);
        if (!position || std::abs(position->quantity) < 1e-8) {
            continue;
        }
        
        if (position->quantity > 0) { // Long position
            // Update highest price if new price is higher
            if (price > stopLoss.highestPrice) {
                stopLoss.highestPrice = price;
                // Adjust stop loss price
                stopLoss.stopPrice = price * (1.0 - stopLoss.trailingDistance);
                stopLoss.lastUpdated = std::chrono::system_clock::now();
            }
        } else { // Short position
            // Update lowest price if new price is lower
            if (price < stopLoss.lowestPrice || stopLoss.lowestPrice == 0) {
                stopLoss.lowestPrice = price;
                // Adjust stop loss price
                stopLoss.stopPrice = price * (1.0 + stopLoss.trailingDistance);
                stopLoss.lastUpdated = std::chrono::system_clock::now();
            }
        }
    }
}

bool StopLossManager::isStopLossTriggered(const StopLossInfo& stopLoss, double currentPrice, double positionSize) {
    if (positionSize > 0) { // Long position
        // Stop loss triggered when price falls below stop price
        return currentPrice <= stopLoss.stopPrice;
    } else { // Short position
        // Stop loss triggered when price rises above stop price
        return currentPrice >= stopLoss.stopPrice;
    }
}

StopLossEvent StopLossManager::createStopLossEvent(const ExchangeId& exchangeId, const TradingPair& tradingPair,
                                                  double positionSize, double entryPrice, double currentPrice, 
                                                  double lossPercentage) {
    StopLossEvent event;
    event.exchangeId = exchangeId;
    event.tradingPair = tradingPair;
    event.positionSize = positionSize;
    event.entryPrice = entryPrice;
    event.triggerPrice = currentPrice;
    event.lossPercentage = lossPercentage;
    event.timestamp = std::chrono::system_clock::now();
    return event;
}

} // namespace risk
}