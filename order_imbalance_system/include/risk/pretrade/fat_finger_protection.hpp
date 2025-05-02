/**
 * Prevents abnormally large or mispriced orders
 *
 * File: fat_finger_protection.hpp
 * Created: 2025-04-29
 */
#pragma once

#include "../risk_types.hpp"
#include "../risk_config.hpp"
#include "../interfaces/data_interface.hpp"

#include <deque>
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace risk {

/**
 * FatFingerProtection prevents abnormally large or mispriced orders.
 * It detects statistical outliers in order parameters.
 */
class FatFingerProtection {
public:
    /**
     * Constructor
     * @param config Risk configuration containing fat finger protection settings
     * @param dataInterface Interface to access market data and order history
     */
    FatFingerProtection(const RiskConfig& config, std::shared_ptr<DataInterface> dataInterface);
    
    /**
     * Initialize the protection
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Check if an order has potential fat finger issues
     * @param signal The trading signal to check
     * @param violations Vector to fill with any violations
     * @return true if no issues, false otherwise
     */
    bool checkFatFinger(const TradingSignal& signal, std::vector<RiskViolation>& violations);
    
    /**
     * Check if an order has potential fat finger issues
     * @param orderRequest The order request to check
     * @param violations Vector to fill with any violations
     * @return true if no issues, false otherwise
     */
    bool checkFatFinger(const OrderRequest& orderRequest, std::vector<RiskViolation>& violations);
    
    /**
     * Record an executed order for historical analysis
     * @param order The executed order
     */
    void recordOrder(const OrderRequest& order);
    
    /**
     * Update fat finger protection settings
     * @param fatFingerConfig New fat finger protection configuration
     * @return true if update was successful, false otherwise
     */
    bool updateSettings(const FatFingerConfig& fatFingerConfig);
    
    /**
     * Check if an order requires additional verification due to high value
     * @param tradingPair The trading pair
     * @param price The order price
     * @param quantity The order quantity
     * @return true if additional verification is required, false otherwise
     */
    bool requiresAdditionalVerification(const TradingPair& tradingPair, double price, double quantity) const;
    
    /**
     * Get the typical order size for a trading pair
     * @param tradingPair The trading pair
     * @return The typical order size based on historical data
     */
    double getTypicalOrderSize(const TradingPair& tradingPair) const;

private:
    std::shared_ptr<DataInterface> m_dataInterface;
    RiskConfig m_config;
    FatFingerConfig m_fatFingerConfig;
    
    // Thread safety
    mutable std::mutex m_configMutex;
    mutable std::mutex m_historyMutex;
    
    // Order history for typical size calculation
    static constexpr size_t MAX_HISTORY_SIZE = 100;
    std::unordered_map<std::string, std::deque<double>> m_orderSizeHistory;
    std::unordered_map<std::string, double> m_typicalOrderSizeCache;
    
    // Helper methods
    bool checkPriceDeviation(const TradingPair& tradingPair, double price, std::vector<RiskViolation>& violations);
    bool checkSizeAnomaly(const TradingPair& tradingPair, double quantity, std::vector<RiskViolation>& violations);
    bool checkNotionalValue(const TradingPair& tradingPair, double price, double quantity, std::vector<RiskViolation>& violations);
    void recalculateTypicalSize(const std::string& symbol);
    double getCurrentMarketPrice(const TradingPair& tradingPair) const;
    double calculateNotionalValue(double price, double quantity) const;
};

} // namespace risk