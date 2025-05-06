/**
 * @file execution_service.cpp
 * @brief Implementation of the main execution service interface
 * 
 * Created on: May 5, 2025
 */

 #include "execution/execution_service.hpp"
 #include <chrono>
 #include <sstream>
 #include <nlohmann/json.hpp>
 #include <thread>
 
 namespace trading {
 namespace execution {
 
 // Struct definitions for service interfaces
 struct TradingSignal {
     std::string signal_id;
     std::string trading_pair;
     OrderSide direction;
     double strength;
     double target_size;
     std::chrono::milliseconds signal_ttl;
     std::string strategy_id;
     std::string strategy_version;
     std::map<std::string, double> signal_metrics;
 };
 
 struct ExecutionResponse {
     std::string client_order_id;
     std::string exchange_order_id;
     bool success;
     std::string error_message;
     std::string order_status;
     std::chrono::system_clock::time_point timestamp;
 };
 
 struct OrderStatusResponse {
     Order order;
     bool found;
     std::string error_message;
 };
 
 struct CancelResponse {
     std::string client_order_id;
     std::string exchange_order_id;
     bool success;
     std::string error_message;
     std::chrono::system_clock::time_point timestamp;
 };
 
 ExecutionService::ExecutionService(
     std::shared_ptr<OrderGenerator> order_generator,
     std::shared_ptr<SmartOrderRouter> smart_order_router,
     std::shared_ptr<OrderTypeSelector> order_type_selector,
     std::shared_ptr<ExecutionAlgorithmFactory> execution_algorithm_factory,
     std::shared_ptr<OrderLifecycleTracker> order_lifecycle_tracker,
     std::shared_ptr<FillReconciliation> fill_reconciliation,
     std::shared_ptr<ApiClientManager> api_client_manager)
     : order_generator_(std::move(order_generator)),
       smart_order_router_(std::move(smart_order_router)),
       order_type_selector_(std::move(order_type_selector)),
       execution_algorithm_factory_(std::move(execution_algorithm_factory)),
       order_lifecycle_tracker_(std::move(order_lifecycle_tracker)),
       fill_reconciliation_(std::move(fill_reconciliation)),
       api_client_manager_(std::move(api_client_manager)),
       is_initialized_(false),
       is_running_(false) {
 }
 
 ExecutionService::~ExecutionService() {
     if (is_running_) {
         shutdown();
     }
 }
 
 bool ExecutionService::initialize() {
     std::lock_guard<std::mutex> lock(mutex_);
     
     if (is_initialized_) {
         return true; // Already initialized
     }
     
     // Initialize all components
     bool success = true;
     
     // Initialize API client manager first
     success &= api_client_manager_->initialize();
     if (!success) {
         return false;
     }
     
     // Initialize other components
     success &= order_lifecycle_tracker_->initialize();
     success &= fill_reconciliation_->initialize();
     
     // Start reconciliation process
     if (success) {
         is_initialized_ = true;
         is_running_ = true;
         
         // Start the fill reconciliation process
         fill_reconciliation_->start();
     }
     
     return success;
 }
 
 void ExecutionService::shutdown() {
     std::lock_guard<std::mutex> lock(mutex_);
     
     if (!is_running_) {
         return; // Already shut down
     }
     
     // Stop fill reconciliation
     fill_reconciliation_->stop();
     
     // Shutdown components in reverse order
     api_client_manager_->shutdown();
     order_lifecycle_tracker_->shutdown();
     
     is_running_ = false;
 }
 
 ExecutionResponse ExecutionService::executeSignal(const TradingSignal& signal) {
     // Validate service state
     if (!is_running_) {
         return ExecutionResponse{
             "", "", false, "Execution service not running", 
             "REJECTED", std::chrono::system_clock::now()
         };
     }
     
     // Create order from signal
     Order order = createOrderFromSignal(signal);
     
     // Validate order
     if (!validateOrder(order)) {
         return ExecutionResponse{
             order.getClientOrderId(), "", false, 
             "Invalid order parameters", "REJECTED", 
             std::chrono::system_clock::now()
         };
     }
     
     // Execute order with appropriate algorithm
     return executeOrderWithAlgorithm(order);
 }
 
 std::future<ExecutionResponse> ExecutionService::executeSignalAsync(const TradingSignal& signal) {
     return std::async(std::launch::async, [this, signal]() {
         return this->executeSignal(signal);
     });
 }
 
 ExecutionResponse ExecutionService::placeOrder(const Order& order) {
     // Validate service state
     if (!is_running_) {
         return ExecutionResponse{
             "", "", false, "Execution service not running", 
             "REJECTED", std::chrono::system_clock::now()
         };
     }
     
     // Validate order
     if (!validateOrder(order)) {
         return ExecutionResponse{
             order.getClientOrderId(), "", false, 
             "Invalid order parameters", "REJECTED", 
             std::chrono::system_clock::now()
         };
     }
     
     // Select exchange
     std::string exchange_id = smart_order_router_->selectExchange(order.getTradingPair(), order.getType());
     
     // Check if valid exchange was found
     if (exchange_id.empty()) {
         return ExecutionResponse{
             order.getClientOrderId(), "", false, 
             "No suitable exchange found", "REJECTED", 
             std::chrono::system_clock::now()
         };
     }
     
     // Create a copy of the order with the exchange ID
     Order exchange_order = order;
     exchange_order.setExchangeId(exchange_id);
     
     // Register the order with the lifecycle tracker
     order_lifecycle_tracker_->trackOrder(exchange_order);
     
     // Get the exchange API client
     auto api_client = api_client_manager_->getClient(exchange_id);
     if (!api_client) {
         order_lifecycle_tracker_->updateOrderState(
             exchange_order.getClientOrderId(), 
             OrderState::REJECTED, 
             "Exchange API client not available"
         );
         
         return ExecutionResponse{
             exchange_order.getClientOrderId(), "", false, 
             "Exchange API client not available", "REJECTED", 
             std::chrono::system_clock::now()
         };
     }
     
     // Submit the order to the exchange
     auto result = api_client->placeOrder(exchange_order);
     
     // Update order state based on exchange response
     if (result.success) {
         // Update with exchange order ID
         order_lifecycle_tracker_->updateOrderWithExchangeId(
             exchange_order.getClientOrderId(),
             result.exchange_order_id
         );
         
         // Update order state to OPEN
         order_lifecycle_tracker_->updateOrderState(
             exchange_order.getClientOrderId(),
             OrderState::OPEN
         );
     } else {
         // Update order state to REJECTED
         order_lifecycle_tracker_->updateOrderState(
             exchange_order.getClientOrderId(),
             OrderState::REJECTED,
             result.error_message
         );
     }
     
     // Prepare response
     ExecutionResponse response;
     response.client_order_id = exchange_order.getClientOrderId();
     response.exchange_order_id = result.exchange_order_id;
     response.success = result.success;
     response.error_message = result.error_message;
     response.order_status = result.success ? "OPEN" : "REJECTED";
     response.timestamp = std::chrono::system_clock::now();
     
     return response;
 }
 
 CancelResponse ExecutionService::cancelOrder(const std::string& client_order_id) {
     // Validate service state
     if (!is_running_) {
         return CancelResponse{
             client_order_id, "", false, 
             "Execution service not running", 
             std::chrono::system_clock::now()
         };
     }
     
     // Get order details from lifecycle tracker
     auto order_opt = order_lifecycle_tracker_->getOrder(client_order_id);
     if (!order_opt) {
         return CancelResponse{
             client_order_id, "", false, 
             "Order not found", 
             std::chrono::system_clock::now()
         };
     }
     
     // Get the order
     const auto& order = *order_opt;
     
     // Check if order can be canceled
     if (!order.canCancel()) {
         return CancelResponse{
             client_order_id, order.getExchangeOrderId(), false, 
             "Order cannot be canceled in current state: " + order.toString(), 
             std::chrono::system_clock::now()
         };
     }
     
     // Get exchange API client
     auto api_client = api_client_manager_->getClient(order.getExchangeId());
     if (!api_client) {
         return CancelResponse{
             client_order_id, order.getExchangeOrderId(), false, 
             "Exchange API client not available", 
             std::chrono::system_clock::now()
         };
     }
     
     // Update order state to PENDING_CANCEL
     order_lifecycle_tracker_->updateOrderState(
         client_order_id,
         OrderState::PENDING_CANCEL
     );
     
     // Send cancel request to exchange
     auto result = api_client->cancelOrder(order);
     
     // Update order state based on exchange response
     if (result.success) {
         order_lifecycle_tracker_->updateOrderState(
             client_order_id,
             OrderState::CANCELED
         );
     } else {
         // Revert to previous state if cancel failed
         order_lifecycle_tracker_->revertToPreviousState(client_order_id);
     }
     
     // Prepare response
     CancelResponse response;
     response.client_order_id = client_order_id;
     response.exchange_order_id = order.getExchangeOrderId();
     response.success = result.success;
     response.error_message = result.error_message;
     response.timestamp = std::chrono::system_clock::now();
     
     return response;
 }
 
 CancelResponse ExecutionService::cancelOrderByExchangeId(
     const std::string& exchange_order_id, 
     const std::string& exchange_id) {
     
     // Validate service state
     if (!is_running_) {
         return CancelResponse{
             "", exchange_order_id, false, 
             "Execution service not running", 
             std::chrono::system_clock::now()
         };
     }
     
     // Get client order ID from exchange order ID
     auto client_order_id = order_lifecycle_tracker_->getClientOrderId(exchange_order_id, exchange_id);
     if (client_order_id.empty()) {
         return CancelResponse{
             "", exchange_order_id, false, 
             "Order not found", 
             std::chrono::system_clock::now()
         };
     }
     
     // Use the standard cancel method
     return cancelOrder(client_order_id);
 }
 
 std::vector<CancelResponse> ExecutionService::cancelAllOrdersForSymbol(
     const std::string& trading_pair,
     const std::string& exchange_id) {
     
     std::vector<CancelResponse> responses;
     
     // Validate service state
     if (!is_running_) {
         responses.push_back(CancelResponse{
             "", "", false, 
             "Execution service not running", 
             std::chrono::system_clock::now()
         });
         return responses;
     }
     
     // Get all active orders for the trading pair
     auto active_orders = order_lifecycle_tracker_->getActiveOrdersForSymbol(trading_pair, exchange_id);
     
     // Cancel each order
     for (const auto& order : active_orders) {
         responses.push_back(cancelOrder(order.getClientOrderId()));
     }
     
     // If no orders were found, add a response indicating that
     if (responses.empty()) {
         responses.push_back(CancelResponse{
             "", "", true, 
             "No active orders found for " + trading_pair, 
             std::chrono::system_clock::now()
         });
     }
     
     return responses;
 }
 
 OrderStatusResponse ExecutionService::getOrderStatus(const std::string& client_order_id) {
     // Validate service state
     if (!is_running_) {
         return OrderStatusResponse{
             Order{}, false, "Execution service not running"
         };
     }
     
     // Get order from lifecycle tracker
     auto order_opt = order_lifecycle_tracker_->getOrder(client_order_id);
     if (!order_opt) {
         return OrderStatusResponse{
             Order{}, false, "Order not found"
         };
     }
     
     // For active orders, refresh status from exchange
     const auto& order = *order_opt;
     if (order.isActive()) {
         // Get exchange API client
         auto api_client = api_client_manager_->getClient(order.getExchangeId());
         if (api_client) {
             // Query exchange for latest status
             api_client->getOrderStatus(order);
             // Refresh our local copy after potential update
             order_opt = order_lifecycle_tracker_->getOrder(client_order_id);
         }
     }
     
     // Prepare response
     OrderStatusResponse response;
     response.order = *order_opt;
     response.found = true;
     response.error_message = "";
     
     return response;
 }
 
 std::vector<Order> ExecutionService::getActiveOrders() {
     if (!is_running_) {
         return {};
     }
     
     return order_lifecycle_tracker_->getActiveOrders();
 }
 
 std::vector<Order> ExecutionService::getFilledOrders(
     const std::chrono::system_clock::time_point& start_time,
     const std::chrono::system_clock::time_point& end_time) {
     
     if (!is_running_) {
         return {};
     }
     
     return order_lifecycle_tracker_->getFilledOrders(start_time, end_time);
 }
 
 std::string ExecutionService::getHealthStatus() const {
     std::lock_guard<std::mutex> lock(mutex_);
     
     nlohmann::json health;
     health["service"] = "execution_service";
     health["initialized"] = is_initialized_;
     health["running"] = is_running_;
     
     // Add component statuses
     health["components"]["api_client_manager"] = api_client_manager_->getHealthStatus();
     health["components"]["order_lifecycle_tracker"] = order_lifecycle_tracker_->getHealthStatus();
     health["components"]["fill_reconciliation"] = fill_reconciliation_->getHealthStatus();
     
     // Add exchange connection statuses
     health["exchanges"] = api_client_manager_->getExchangeStatuses();
     
     // Add metrics
     health["metrics"]["active_orders"] = order_lifecycle_tracker_->getActiveOrderCount();
     health["metrics"]["orders_placed_last_hour"] = order_lifecycle_tracker_->getOrdersPlacedInLastHour();
     health["metrics"]["fills_last_hour"] = order_lifecycle_tracker_->getFillsInLastHour();
     
     return health.dump(2);
 }
 
 // Private helper methods
 
 Order ExecutionService::createOrderFromSignal(const TradingSignal& signal) {
     // Use order generator to create an order based on the signal
     OrderGeneratorParams params;
     params.trading_pair = signal.trading_pair;
     params.signal_strength = signal.strength;
     params.target_size = signal.target_size;
     params.signal_id = signal.signal_id;
     params.strategy_id = signal.strategy_id;
     params.side = signal.direction;
     
     // Generate the order
     auto order = order_generator_->generateOrder(params);
     
     // Select the best exchange
     std::string exchange_id = smart_order_router_->selectExchange(
         signal.trading_pair, order.getType());
     order.setExchangeId(exchange_id);
     
     // Select the most appropriate order type
     auto selected_type = order_type_selector_->selectOrderType(order, signal.signal_metrics);
     order.setType(selected_type);
     
     return order;
 }
 
 bool ExecutionService::validateOrder(const Order& order) {
     // Basic validation checks
     if (order.getTradingPair().empty()) {
         return false;
     }
     
     if (order.getQuantity() <= 0.0) {
         return false;
     }
     
     // Price validation for limit orders
     if (order.getType() == OrderType::LIMIT && order.getPrice() <= 0.0) {
         return false;
     }
     
     // More complex validation checks can be added here
     
     return true;
 }
 
 ExecutionResponse ExecutionService::executeOrderWithAlgorithm(const Order& order) {
     // Determine if we need a specialized execution algorithm
     if (order.getType() == OrderType::MARKET || order.getType() == OrderType::LIMIT) {
         // Simple orders can be placed directly
         return placeOrder(order);
     } else {
         // Complex orders need a specialized algorithm
         auto algorithm = execution_algorithm_factory_->createAlgorithm(order.getType());
         if (!algorithm) {
             return ExecutionResponse{
                 order.getClientOrderId(), "", false, 
                 "No algorithm available for order type", "REJECTED", 
                 std::chrono::system_clock::now()
             };
         }
         
         // Execute using the algorithm
         return algorithm->execute(order, *this);
     }
 }
 
 } // namespace execution
 } // namespace trading