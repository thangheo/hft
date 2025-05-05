/**
 * Interface for communication with the execution engine
 *
 * File: execution_interface.hpp
 * Created: 2025-04-29
 */
#pragma once

#include "../risk_types.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace trading {
namespace risk {

/**
 * ExecutionInterface provides communication with the Execution Engine.
 * It allows order management and receives execution reports.
 */
class ExecutionInterface {
public:
    /**
     * Constructor
     */
    ExecutionInterface();
    
    /**
     * Destructor
     */
    virtual ~ExecutionInterface() = default;
    
    /**
     * Initialize the interface
     * @return true if initialization was successful, false otherwise
     */
    virtual bool initialize() = 0;
    
    /**
     * Submit an order
     * @param orderRequest The order request
     * @return Order ID if successful, empty string otherwise
     */
    virtual std::string submitOrder(const OrderRequest& orderRequest) = 0;
    
    /**
     * Cancel an order
     * @param orderId The order identifier
     * @return true if cancellation was successful, false otherwise
     */
    virtual bool cancelOrder(const std::string& orderId) = 0;
    
    /**
     * Cancel all orders for a specific exchange
     * @param exchangeId The exchange identifier
     * @return Number of orders cancelled
     */
    virtual int cancelAllOrders(const ExchangeId& exchangeId) = 0;
    
    /**
     * Cancel all orders for a specific trading pair
     * @param exchangeId The exchange identifier
     * @param tradingPair The trading pair
     * @return Number of orders cancelled
     */
    virtual int cancelAllOrders(const ExchangeId& exchangeId, const TradingPair& tradingPair) = 0;
    
    /**
     * Get order status
     * @param orderId The order identifier
     * @return JSON string containing order status
     */
    virtual std::string getOrderStatus(const std::string& orderId) = 0;
    
    /**
     * Register a callback for execution reports
     * @param callback The function to call when execution reports are received
     */
    virtual void registerExecutionReportCallback(const std::function<void(const ExecutionReport&)>& callback) = 0;
    
    /**
     * Get all open orders
     * @param exchangeId Optional exchange ID to filter orders
     * @return Vector of open order information
     */
    virtual std::vector<OrderRequest> getOpenOrders(const std::optional<ExchangeId>& exchangeId = std::nullopt) = 0;
    
    /**
     * Check if an exchange is available
     * @param exchangeId The exchange identifier
     * @return true if available, false otherwise
     */
    virtual bool isExchangeAvailable(const ExchangeId& exchangeId) = 0;
    
    /**
     * Get exchange status
     * @param exchangeId The exchange identifier
     * @return JSON string containing exchange status
     */
    virtual std::string getExchangeStatus(const ExchangeId& exchangeId) = 0;
};

} // namespace risk
}