/**
 * Interface for accessing market data
 *
 * File: data_interface.hpp
 * Created: 2025-04-29
 */

#pragma once

#include "risk_types.hpp"
// #include "core/market_data.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>

namespace trading {
namespace risk {

/**
 * Interface for accessing market data
 */
class DataInterface {
public:
    /**
     * Constructor
     */
    DataInterface();
    
    /**
     * Virtual destructor
     */
    virtual ~DataInterface() = default;

    /**
     * Initialize the data interface
     * 
     * @return true if successful, false otherwise
     */
    virtual bool initialize() = 0;
    
    /**
     * Get market data for a trading pair
     * 
     * @param tradingPair The trading pair
     * @param exchangeId Optional exchange ID
     * @return MarketData structure
     */
    // virtual MarketData getMarketData(const std::string& tradingPair, const std::optional<std::string>& exchangeId = std::nullopt) = 0;
    // In data_interface.hpp
    virtual std::optional<MarketData> getMarketData(const std::string& tradingPair,
                                               const std::optional<std::string>& exchangeId = std::nullopt) = 0;
    /**
     * Get current price for a trading pair
     * 
     * @param tradingPair The trading pair
     * @param exchangeId Optional exchange ID
     * @return Current price
     */
    virtual double getCurrentPrice(const std::string& tradingPair, const std::optional<std::string>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get historical prices for a trading pair
     * 
     * @param tradingPair The trading pair
     * @param interval Time interval (e.g., "1m", "1h", "1d")
     * @param count Number of data points to retrieve
     * @param exchangeId Optional exchange ID
     * @return Vector of timestamp-price pairs
     */
    virtual std::vector<std::pair<Timestamp, double>> getHistoricalPrices(
        const std::string& tradingPair,
        const std::string& interval,
        int count,
        const std::optional<std::string>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get volatility for a trading pair
     * 
     * @param tradingPair The trading pair
     * @param timeframe Timeframe for volatility calculation (e.g., "1h", "1d", "7d")
     * @param exchangeId Optional exchange ID
     * @return Volatility value
     */
    virtual double getVolatility(const std::string& tradingPair, const std::string& timeframe, const std::optional<std::string>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get order history for a trading pair
     * 
     * @param exchangeId Exchange ID
     * @param tradingPair The trading pair
     * @param limit Maximum number of orders to retrieve
     * @return Vector of order requests
     */
    virtual std::vector<OrderRequest> getOrderHistory(const core::ExchangeId& exchangeId, const std::string& tradingPair, int limit) = 0;
    
    /**
     * Get exchange balances
     * 
     * @param exchangeId Exchange ID
     * @return Map of asset to balance
     */
    virtual std::map<std::string, double> getExchangeBalances(const core::ExchangeId& exchangeId) = 0;
    
    /**
     * Register callback for market data updates
     * 
     * @param callback Function to call when market data is updated
     */
    virtual void registerMarketDataCallback(const std::function<void(const MarketData&)>& callback) = 0;
    
    /**
     * Subscribe to market data updates
     * 
     * @param tradingPair The trading pair
     * @param exchangeId Optional exchange ID
     * @return true if successful, false otherwise
     */
    virtual bool subscribeMarketData(const std::string& tradingPair, const std::optional<std::string>& exchangeId = std::nullopt) = 0;
    
    /**
     * Unsubscribe from market data updates
     * 
     * @param tradingPair The trading pair
     * @param exchangeId Optional exchange ID
     * @return true if successful, false otherwise
     */
    virtual bool unsubscribeMarketData(const std::string& tradingPair, const std::optional<std::string>& exchangeId = std::nullopt) = 0;
    
    /**
     * Get trading fees
     * 
     * @param exchangeId Exchange ID
     * @param tradingPair Optional trading pair
     * @return Map of fee types to fee values
     */
    virtual std::map<std::string, double> getTradingFees(const core::ExchangeId& exchangeId, const std::optional<std::string>& tradingPair = std::nullopt) = 0;
};

/**
 * Factory function to create DataInterface
 * 
 * @return Unique pointer to DataInterface implementation
 */
std::unique_ptr<DataInterface> createDataInterface();

} // namespace risk
} // namespace trading