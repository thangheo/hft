#ifndef STRATEGY_INTERFACE_H
#define STRATEGY_INTERFACE_H

#include <string>
#include <memory>
#include "../common/market_data.h"
#include "../common/order_data.h"

class IStrategy {
public:
    virtual ~IStrategy() = default;

    /**
     * @brief Initialize the strategy with necessary parameters.
     * @param config_file Path to the configuration file.
     * @return True if initialization is successful.
     */
    virtual bool initialize(const std::string& config_file) = 0;

    /**
     * @brief Process incoming market data.
     * @param data The market data to process.
     */
    virtual void onMarketData(const MarketData& data) = 0;

    /**
     * @brief Handle order execution reports.
     * @param order The order data.
     */
    virtual void onOrderUpdate(const Order& order) = 0;

    /**
     * @brief Cleanup resources when the strategy is terminated.
     */
    virtual void shutdown() = 0;
};

#endif // STRATEGY_INTERFACE_H
