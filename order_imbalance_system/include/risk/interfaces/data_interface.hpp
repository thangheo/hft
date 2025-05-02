/**
 * Interface for accessing market data
 *
 * File: data_interface.hpp
 * Created: 2025-04-29
 */
#pragma once

#include "../risk_types.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

namespace risk {

/**
 * DataInterface provides access to market data and historical information.
 * It abstracts the data retrieval operations for the Risk Management System.
 */
class DataInterface {
public:
    /**
     * Constructor
     */
    DataInterface();
    
    /**
     * Destructor
     */
    virtual ~DataInterface() = default;
    
    /**
     * Initialize the interface
     * @return true if initialization was successful, false otherwise
     */
    virtual bool initialize() = 0;
    
    /**
     * Get current market data
     * @param tradingPair The trading pair
     * @param exchangeId Optional exchange ID to filter data
     * @return MarketData object
     */
    virtual MarketData getMarketData(const TradingPair& tradingPair, 
                                   const std::optional<ExchangeId>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get current price
     * @param tradingPair The trading pair
     * @param exchangeId Optional exchange ID to filter data
     * @return Current price
     */
    virtual double getCurrentPrice(const TradingPair& tradingPair,
                                 const std::optional<ExchangeId>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get historical prices
     * @param tradingPair The trading pair
     * @param interval The time interval
     * @param count The number of data points
     * @param exchangeId Optional exchange ID to filter data
     * @return Vector of price data points
     */
    virtual std::vector<std::pair<Timestamp, double>> getHistoricalPrices(
        const TradingPair& tradingPair,
        const std::string& interval,
        int count,
        const std::optional<ExchangeId>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get volatility
     * @param tradingPair The trading pair
     * @param timeframe The timeframe for volatility calculation
     * @param exchangeId Optional exchange ID to filter data
     * @return Volatility value
     */
    virtual double getVolatility(const TradingPair& tradingPair,
                              const std::string& timeframe,
                              const std::optional<ExchangeId>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get order history
     * @param exchangeId The exchange identifier
     * @param tradingPair The trading pair
     * @param limit Maximum number of orders to retrieve
     * @return Vector of historical orders
     */
    virtual std::vector<OrderRequest> getOrderHistory(const ExchangeId& exchangeId,
                                                   const TradingPair& tradingPair,
                                                   int limit) = 0;
    
    /**
     * Get exchange balances
     * @param exchangeId The exchange identifier
     * @return Map of asset to balance
     */
    virtual std::map<std::string, double> getExchangeBalances(const ExchangeId& exchangeId) = 0;
    
    /**
     * Register a callback for market data updates
     * @param callback The function to call when market data is updated
     */
    virtual void registerMarketDataCallback(const std::function<void(const MarketData&)>& callback) = 0;
    
    /**
     * Subscribe to market data for a trading pair
     * @param tradingPair The trading pair
     * @param exchangeId Optional exchange ID to filter data
     * @return true if subscription was successful, false otherwise
     */
    virtual bool subscribeMarketData(const TradingPair& tradingPair,
                                   const std::optional<ExchangeId>& exchangeId = std::nullopt) = 0;
    
    /**
     * Unsubscribe from market data for a trading pair
     * @param tradingPair The trading pair
     * @param exchangeId Optional exchange ID to filter data
     * @return true if unsubscription was successful, false otherwise
     */
    virtual bool unsubscribeMarketData(const TradingPair& tradingPair,
                                     const std::optional<ExchangeId>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get trading fees
     * @param exchangeId The exchange identifier
     * @param tradingPair Optional trading pair to get specific fees
     * @return Map of fee types to values
     */
    virtual std::map<std::string, double> getTradingFees(const ExchangeId& exchangeId,
                                                      const std::optional<TradingPair>& tradingPair = std::nullopt) = 0;
};

} // namespace risk
