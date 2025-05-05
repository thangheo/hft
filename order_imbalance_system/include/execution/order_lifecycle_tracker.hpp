/**
 * @file order_lifecycle_tracker.hpp
 * @brief Order lifecycle tracking and management
 * 
 * @details 
 * Tracks the state and history of orders as they progress through
 * their lifecycle from creation to final status.
 * 
 * Created on: May 5, 2025
 */
#pragma once

#include "execution/models/order.hpp"
#include "execution/models/execution_report.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>

namespace trading {
namespace execution {

// Forward declarations
class OrderLifecycleTracker;

/**
 * @brief Callback for order state changes
 */
using OrderStateChangeCallback = std::function<void(const Order&, OrderState, OrderState)>;

/**
 * @brief Manages the lifecycle of orders through the system
 */
class OrderLifecycleTracker {
public:
    /**
     * @brief Constructor
     */
    OrderLifecycleTracker();
    
    /**
     * @brief Destructor
     */
    ~OrderLifecycleTracker() = default;
    
    /**
     * @brief Add a new order to track
     * 
     * @param order The order to track
     * @return true if successful, false if order already exists
     */
    bool addOrder(const Order& order);
    
    /**
     * @brief Update an order's state
     * 
     * @param client_order_id The client order ID
     * @param new_state The new state
     * @param exchange_order_id Optional exchange order ID (if not already set)
     * @return true if successful, false if order not found
     */
    bool updateOrderState(const std::string& client_order_id, 
                         OrderState new_state,
                         const std::string& exchange_order_id = "");
    
    /**
     * @brief Record a fill for an order
     * 
     * @param client_order_id The client order ID
     * @param fill The fill information
     * @return true if successful, false if order not found
     */
    bool recordFill(const std::string& client_order_id, const Fill& fill);
    
    /**
     * @brief Get an order by client order ID
     * 
     * @param client_order_id The client order ID
     * @return The order, or nullptr if not found
     */
    std::shared_ptr<Order> getOrderByClientId(const std::string& client_order_id) const;
    
    /**
     * @brief Get an order by exchange order ID
     * 
     * @param exchange_order_id The exchange order ID
     * @return The order, or nullptr if not found
     */
    std::shared_ptr<Order> getOrderByExchangeId(const std::string& exchange_order_id) const;
    
    /**
     * @brief Get all active orders
     * 
     * @return Vector of active orders
     */
    std::vector<std::shared_ptr<Order>> getActiveOrders() const;
    
    /**
     * @brief Get all orders in a specific state
     * 
     * @param state The state to filter by
     * @return Vector of orders in the requested state
     */
    std::vector<std::shared_ptr<Order>> getOrdersByState(OrderState state) const;
    
    /**
     * @brief Get all orders for a trading pair
     * 
     * @param symbol The trading pair
     * @return Vector of orders for the trading pair
     */
    std::vector<std::shared_ptr<Order>> getOrdersBySymbol(const std::string& symbol) const;
    
    /**
     * @brief Register a callback for order state changes
     * 
     * @param callback The callback function
     */
    void registerStateChangeCallback(OrderStateChangeCallback callback);
    
    /**
     * @brief Check for orders that have timed out (no update for a certain period)
     * 
     * @param timeout_threshold_ms Timeout threshold in milliseconds
     * @return Vector of timed out orders
     */
    std::vector<std::shared_ptr<Order>> checkForTimedOutOrders(int64_t timeout_threshold_ms);
    
    /**
     * @brief Purge old orders from memory
     * 
     * @param age_threshold_ms Age threshold in milliseconds
     * @return Number of orders purged
     */
    int purgeOldOrders(int64_t age_threshold_ms);
    
private:
    // Maps for quick lookup
    std::unordered_map<std::string, std::shared_ptr<Order>> orders_by_client_id_;
    std::unordered_map<std::string, std::string> exchange_to_client_id_map_;
    
    // Callbacks
    std::vector<OrderStateChangeCallback> state_change_callbacks_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper methods
    void notifyStateChange(const Order& order, OrderState old_state, OrderState new_state);
};

} // namespace execution
} // namespace trading