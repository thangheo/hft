/**
 * @file execution_service.hpp
 * @brief Main service interface for the execution engine
 * 
 * @details 
 * Provides the main service interface for the execution engine,
 * coordinating between different components to execute trading operations.
 * 
 * Created on: May 5, 2025
 */
#pragma once

#include "execution/models/order.hpp"
#include "execution/models/execution_report.hpp"
#include "execution/order_generator.hpp"
#include "execution/smart_order_router.hpp"
#include "execution/order_type_selector.hpp"
#include "execution/execution_algorithm_factory.hpp"
#include "execution/order_lifecycle_tracker.hpp"
#include "execution/fill_reconciliation.hpp"
#include "execution/api_client_manager.hpp"

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <future>

namespace trading {
namespace execution {

// Forward declarations
struct TradingSignal;
struct ExecutionResponse;
struct OrderStatusResponse;
struct CancelResponse;

/**
 * @brief Main service interface for the Execution Engine
 */
class ExecutionService {
public:
    /**
     * @brief Constructor
     * 
     * @param order_generator Order generator component
     * @param smart_order_router Smart order router component
     * @param order_type_selector Order type selector component
     * @param execution_algorithm_factory Execution algorithm factory
     * @param order_lifecycle_tracker Order lifecycle tracker
     * @param fill_reconciliation Fill reconciliation
     * @param api_client_manager API client manager
     */
    ExecutionService(
        std::shared_ptr<OrderGenerator> order_generator,
        std::shared_ptr<SmartOrderRouter> smart_order_router,
        std::shared_ptr<OrderTypeSelector> order_type_selector,
        std::shared_ptr<ExecutionAlgorithmFactory> execution_algorithm_factory,
        std::shared_ptr<OrderLifecycleTracker> order_lifecycle_tracker,
        std::shared_ptr<FillReconciliation> fill_reconciliation,
        std::shared_ptr<ApiClientManager> api_client_manager
    );
    
    /**
     * @brief Destructor
     */
    ~ExecutionService();
    
    /**
     * @brief Initialize the service
     * 
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Shutdown the service
     */
    void shutdown();
    
    /**
     * @brief Execute a trading signal
     * 
     * @param signal The trading signal to execute
     * @return ExecutionResponse containing execution status and details
     */
    ExecutionResponse executeSignal(const TradingSignal& signal);
    
    /**
     * @brief Execute a trading signal asynchronously
     * 
     * @param signal The trading signal to execute
     * @return std::future<ExecutionResponse> Future for the execution response
     */
    std::future<ExecutionResponse> executeSignalAsync(const TradingSignal& signal);
    
    /**
     * @brief Place an order directly
     * 
     * @param order The order to place
     * @return ExecutionResponse containing execution status and details
     */
    ExecutionResponse placeOrder(const Order& order);
    
    /**
     * @brief Cancel an order
     * 
     * @param client_order_id The client order ID to cancel
     * @return CancelResponse containing cancellation status and details
     */
    CancelResponse cancelOrder(const std::string& client_order_id);
    
    /**
     * @brief Cancel an order by exchange order ID
     * 
     * @param exchange_order_id The exchange order ID to cancel
     * @param exchange_id The exchange ID
     * @return CancelResponse containing cancellation status and details
     */
    CancelResponse cancelOrderByExchangeId(const std::string& exchange_order_id, 
                                          const std::string& exchange_id);
    
    /**
     * @brief Cancel all orders for a trading pair
     * 
     * @param trading_pair The trading pair
     * @param exchange_id Optional exchange ID to filter by
     * @return Vector of CancelResponse for each order
     */
    std::vector<CancelResponse> cancelAllOrdersForSymbol(const std::string& trading_pair,
                                                        const std::string& exchange_id = "");
    
    /**
     * @brief Get order status
     * 
     * @param client_order_id The client order ID
     * @return OrderStatusResponse containing order status and details
     */
    OrderStatusResponse getOrderStatus(const std::string& client_order_id);
    
    /**
     * @brief Get all active orders
     * 
     * @return Vector of active orders
     */
    std::vector<Order> getActiveOrders();
    
    /**
     * @brief Get all filled orders within a time window
     * 
     * @param start_time Start time
     * @param end_time End time
     * @return Vector of filled orders
     */
    std::vector<Order> getFilledOrders(
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time);
    
    /**
     * @brief Get health status of execution service
     * 
     * @return Health status as a JSON string
     */
    std::string getHealthStatus() const;
    
private:
    // Component references
    std::shared_ptr<OrderGenerator> order_generator_;
    std::shared_ptr<SmartOrderRouter> smart_order_router_;
    std::shared_ptr<OrderTypeSelector> order_type_selector_;
    std::shared_ptr<ExecutionAlgorithmFactory> execution_algorithm_factory_;
    std::shared_ptr<OrderLifecycleTracker> order_lifecycle_tracker_;
    std::shared_ptr<FillReconciliation> fill_reconciliation_;
    std::shared_ptr<ApiClientManager> api_client_manager_;
    
    // Service state
    bool is_initialized_ = false;
    bool is_running_ = false;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper methods
    Order createOrderFromSignal(const TradingSignal& signal);
    bool validateOrder(const Order& order);
    ExecutionResponse executeOrderWithAlgorithm(const Order& order);
};

} // namespace execution
} // namespace trading