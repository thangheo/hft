/**
 * @file order_lifecycle_tracker.cpp
 * @brief Implementation of order lifecycle tracking and management
 * 
 * @details 
 * Tracks the state and history of orders as they progress through
 * their lifecycle from creation to final status.
 * 
 * Created on: May 5, 2025
 */

 #include "execution/order_lifecycle_tracker.hpp"
 #include <algorithm>
 #include <iostream>
 #include <sstream>
 
 namespace trading {
 namespace execution {
 
 OrderLifecycleTracker::OrderLifecycleTracker() {
     // Nothing to initialize
 }
 
 bool OrderLifecycleTracker::addOrder(const Order& order) {
     std::lock_guard<std::mutex> lock(mutex_);
     
     // Check if order already exists
     if (orders_by_client_id_.find(order.getClientOrderId()) != orders_by_client_id_.end()) {
         return false;  // Order already exists
     }
     
     // Create a shared pointer from the order
     auto order_ptr = std::make_shared<Order>(order);
     
     // Store the order
     orders_by_client_id_[order.getClientOrderId()] = order_ptr;
     
     // If the order has an exchange ID, map it as well
     if (!order.getExchangeOrderId().empty()) {
         exchange_to_client_id_map_[order.getExchangeOrderId()] = order.getClientOrderId();
     }
     
     return true;
 }
 
 bool OrderLifecycleTracker::updateOrderState(const std::string& client_order_id, 
                                            OrderState new_state,
                                            const std::string& exchange_order_id) {
     std::lock_guard<std::mutex> lock(mutex_);
     
     // Find the order
     auto it = orders_by_client_id_.find(client_order_id);
     if (it == orders_by_client_id_.end()) {
         return false;  // Order not found
     }
     
     auto& order = it->second;
     
     // Record the old state
     OrderState old_state = order->getState();
     
     // Update exchange order ID if provided and not already set
     if (!exchange_order_id.empty() && order->getExchangeOrderId().empty()) {
         order->setExchangeOrderId(exchange_order_id);
         exchange_to_client_id_map_[exchange_order_id] = client_order_id;
     }
     
     // Update state based on the new state
     bool state_updated = false;
     switch (new_state) {
         case OrderState::OPEN:
             state_updated = order->open();
             break;
         case OrderState::FILLED:
             // This should normally be done through recordFill, but we'll support it for completeness
             if (order->getFilledQuantity() >= order->getQuantity()) {
                 state_updated = true;  // Already filled
             }
             break;
         case OrderState::CANCELED:
             state_updated = order->cancel();
             break;
         case OrderState::REJECTED:
             state_updated = order->reject("Rejected by exchange");
             break;
         case OrderState::EXPIRED:
             state_updated = order->expire();
             break;
         case OrderState::PENDING:
         case OrderState::PENDING_CANCEL:
         case OrderState::PARTIALLY_FILLED:
             // These states are explicitly set
             if (old_state != new_state) {
                 order.get()->setExchangeOrderId(exchange_order_id);
                 state_updated = true;
             }
             break;
         default:
             state_updated = false;
             break;
     }
     
     // Notify state change
     if (state_updated && old_state != new_state) {
         notifyStateChange(*order, old_state, new_state);
     }
     
     return state_updated;
 }
 
 bool OrderLifecycleTracker::recordFill(const std::string& client_order_id, const Fill& fill) {
     std::lock_guard<std::mutex> lock(mutex_);
     
     // Find the order
     auto it = orders_by_client_id_.find(client_order_id);
     if (it == orders_by_client_id_.end()) {
         return false;  // Order not found
     }
     
     auto& order = it->second;
     
     // Record the old state
     OrderState old_state = order->getState();
     
     // Apply the fill to the order
     bool applied = order->fill(fill);
     
     // Notify state change if state changed
     if (applied && old_state != order->getState()) {
         notifyStateChange(*order, old_state, order->getState());
     }
     
     return applied;
 }
 
 std::shared_ptr<Order> OrderLifecycleTracker::getOrderByClientId(const std::string& client_order_id) const {
     std::lock_guard<std::mutex> lock(mutex_);
     
     auto it = orders_by_client_id_.find(client_order_id);
     if (it != orders_by_client_id_.end()) {
         return it->second;
     }
     
     return nullptr;  // Order not found
 }
 
 std::shared_ptr<Order> OrderLifecycleTracker::getOrderByExchangeId(const std::string& exchange_order_id) const {
     std::lock_guard<std::mutex> lock(mutex_);
     
     // Find the client order ID
     auto it = exchange_to_client_id_map_.find(exchange_order_id);
     if (it == exchange_to_client_id_map_.end()) {
         return nullptr;  // Exchange order ID not found
     }
     
     // Find the order
     auto order_it = orders_by_client_id_.find(it->second);
     if (order_it == orders_by_client_id_.end()) {
         return nullptr;  // Order not found (shouldn't happen)
     }
     
     return order_it->second;
 }
 
 std::vector<std::shared_ptr<Order>> OrderLifecycleTracker::getActiveOrders() const {
     std::lock_guard<std::mutex> lock(mutex_);
     
     std::vector<std::shared_ptr<Order>> active_orders;
     
     for (const auto& pair : orders_by_client_id_) {
         if (pair.second->isActive()) {
             active_orders.push_back(pair.second);
         }
     }
     
     return active_orders;
 }
 
 std::vector<std::shared_ptr<Order>> OrderLifecycleTracker::getOrdersByState(OrderState state) const {
     std::lock_guard<std::mutex> lock(mutex_);
     
     std::vector<std::shared_ptr<Order>> matching_orders;
     
     for (const auto& pair : orders_by_client_id_) {
         if (pair.second->getState() == state) {
             matching_orders.push_back(pair.second);
         }
     }
     
     return matching_orders;
 }
 
 std::vector<std::shared_ptr<Order>> OrderLifecycleTracker::getOrdersBySymbol(const std::string& symbol) const {
     std::lock_guard<std::mutex> lock(mutex_);
     
     std::vector<std::shared_ptr<Order>> matching_orders;
     
     for (const auto& pair : orders_by_client_id_) {
         if (pair.second->getTradingPair() == symbol) {
             matching_orders.push_back(pair.second);
         }
     }
     
     return matching_orders;
 }
 
 void OrderLifecycleTracker::registerStateChangeCallback(OrderStateChangeCallback callback) {
     std::lock_guard<std::mutex> lock(mutex_);
     state_change_callbacks_.push_back(callback);
 }
 
 std::vector<std::shared_ptr<Order>> OrderLifecycleTracker::checkForTimedOutOrders(int64_t timeout_threshold_ms) {
     std::lock_guard<std::mutex> lock(mutex_);
     
     std::vector<std::shared_ptr<Order>> timed_out_orders;
     auto now = std::chrono::system_clock::now();
     
     for (const auto& pair : orders_by_client_id_) {
         auto& order = pair.second;
         
         // Only check active orders that aren't in a terminal state
         if (!order->isActive()) {
             continue;
         }
         
         // Check if order has timed out
         auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
             now - order->getUpdatedTime()).count();
             
         if (elapsed > timeout_threshold_ms) {
             timed_out_orders.push_back(order);
         }
     }
     
     return timed_out_orders;
 }
 
 int OrderLifecycleTracker::purgeOldOrders(int64_t age_threshold_ms) {
     std::lock_guard<std::mutex> lock(mutex_);
     
     int purged_count = 0;
     auto now = std::chrono::system_clock::now();
     std::vector<std::string> to_remove;
     
     // Find orders to purge
     for (const auto& pair : orders_by_client_id_) {
         auto& order = pair.second;
         
         // Only purge orders in terminal states
         if (order->isActive()) {
             continue;
         }
         
         // Check if order is old enough to purge
         auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
             now - order->getUpdatedTime()).count();
             
         if (elapsed > age_threshold_ms) {
             to_remove.push_back(pair.first);
             
             // Also remove from exchange_to_client_id_map_
             if (!order->getExchangeOrderId().empty()) {
                 exchange_to_client_id_map_.erase(order->getExchangeOrderId());
             }
             
             purged_count++;
         }
     }
     
     // Remove the orders
     for (const auto& client_id : to_remove) {
         orders_by_client_id_.erase(client_id);
     }
     
     return purged_count;
 }
 
 void OrderLifecycleTracker::notifyStateChange(const Order& order, OrderState old_state, OrderState new_state) {
     // Call all registered callbacks
     for (const auto& callback : state_change_callbacks_) {
         callback(order, old_state, new_state);
     }
 }
 
 } // namespace execution
 } // namespace trading