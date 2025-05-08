/**
 * @file execution_service_impl.cpp
 * @brief Implementation of the gRPC execution service interface
 * 
 * Created on: May 5, 2025
 */

 #include "execution/grpc/execution_service_impl.hpp"
 #include <chrono>
 #include <sstream>
 #include <grpcpp/grpcpp.h>
 
 namespace trading {
 namespace execution {
 namespace grpc {
 
 ExecutionServiceImpl::ExecutionServiceImpl(std::shared_ptr<ExecutionService> execution_service)
     : execution_service_(std::move(execution_service)) {
     // Ensure service is not null
     if (!execution_service_) {
         throw std::invalid_argument("Execution service cannot be null");
     }
 }
 
 ::grpc::Status ExecutionServiceImpl::ExecuteSignal(
     ::grpc::ServerContext* context,
     const ::trading::proto::TradingSignal* request,
     ::trading::proto::ExecutionResponse* response) {
     
     // Convert proto signal to internal representation
     auto internal_signal = convertTradingSignal(*request);
     
     // Execute the signal
     auto result = execution_service_->executeSignal(internal_signal);
     
     // Convert result to proto response
     *response = createExecutionResponse(result);
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::PlaceOrder(
     ::grpc::ServerContext* context,
     const ::trading::proto::OrderRequest* request,
     ::trading::proto::ExecutionResponse* response) {
     
     // Convert proto order to internal representation
     auto internal_order = convertOrderRequest(*request);
     
     // Place the order
     auto result = execution_service_->placeOrder(internal_order);
     
     // Convert result to proto response
     *response = createExecutionResponse(result);
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::CancelOrder(
     ::grpc::ServerContext* context,
     const ::trading::proto::CancelRequest* request,
     ::trading::proto::CancelResponse* response) {
     
     // Determine which ID type is provided
     if (!request->client_order_id().empty()) {
         // Cancel by client order ID
         auto result = execution_service_->cancelOrder(request->client_order_id());
         
         // Set response fields
         response->set_client_order_id(result.client_order_id);
         response->set_exchange_order_id(result.exchange_order_id);
         response->set_success(result.success);
         response->set_message(result.error_message);
         response->set_timestamp(
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 result.timestamp.time_since_epoch()
             ).count()
         );
     } else if (!request->exchange_order_id().empty()) {
         // Cancel by exchange order ID
         auto result = execution_service_->cancelOrderByExchangeId(
             request->exchange_order_id(),
             request->exchange_id()
         );
         
         // Set response fields
         response->set_client_order_id(result.client_order_id);
         response->set_exchange_order_id(result.exchange_order_id);
         response->set_success(result.success);
         response->set_message(result.error_message);
         response->set_timestamp(
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 result.timestamp.time_since_epoch()
             ).count()
         );
     } else {
         // Neither ID was provided
         response->set_success(false);
         response->set_message("Either client_order_id or exchange_order_id must be provided");
         response->set_timestamp(
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now().time_since_epoch()
             ).count()
         );
     }
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::GetOrderStatus(
     ::grpc::ServerContext* context,
     const ::trading::proto::StatusRequest* request,
     ::trading::proto::OrderStatusResponse* response) {
     
     // Get the order status
     auto result = execution_service_->getOrderStatus(request->client_order_id());
     
     // Set response fields
     response->set_found(result.found);
     response->set_error_message(result.error_message);
     
     // If found, add the order details
     if (result.found) {
         auto* order_proto = response->mutable_order();
         *order_proto = createProtoOrder(result.order);
     }
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::GetActiveOrders(
     ::grpc::ServerContext* context,
     const ::trading::proto::ActiveOrdersRequest* request,
     ::trading::proto::ActiveOrdersResponse* response) {
     
     // Get all active orders
     std::vector<Order> active_orders;
     
     if (!request->trading_pair().empty()) {
         // Filter by trading pair and optional exchange
         active_orders = execution_service_->getActiveOrdersForSymbol(
             request->trading_pair(),
             request->exchange_id()
         );
     } else {
         // Get all active orders
         active_orders = execution_service_->getActiveOrders();
     }
     
     // Add orders to response
     for (const auto& order : active_orders) {
         auto* order_proto = response->add_orders();
         *order_proto = createProtoOrder(order);
     }
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::GetHealthStatus(
     ::grpc::ServerContext* context,
     const ::trading::proto::HealthRequest* request,
     ::trading::proto::HealthResponse* response) {
     
     // Get health status from service
     std::string health_json = execution_service_->getHealthStatus();
     
     // Set response fields
     response->set_status(health_json);
     response->set_is_healthy(true); // Simplified - in practice would parse JSON and determine health
     
     return ::grpc::Status::OK;
 }
 
 // Helper methods for type conversion
 
 trading::execution::TradingSignal ExecutionServiceImpl::convertTradingSignal(
     const ::trading::proto::TradingSignal& proto_signal) {
     
     trading::execution::TradingSignal signal;
     
     // Basic fields
     signal.signal_id = proto_signal.signal_id();
     signal.trading_pair = proto_signal.trading_pair();
     signal.strategy_id = proto_signal.strategy_id();
     signal.strategy_version = proto_signal.strategy_version();
     signal.target_size = proto_signal.target_size();
     signal.strength = proto_signal.strength();
     
     // Convert direction
     signal.direction = (proto_signal.direction() == "buy") 
         ? OrderSide::BUY 
         : OrderSide::SELL;
     
     // Convert TTL
     signal.signal_ttl = std::chrono::milliseconds(proto_signal.signal_ttl_ms());
     
     // Convert metrics
     for (const auto& [key, value] : proto_signal.signal_metrics()) {
         signal.signal_metrics[key] = value;
     }
     
     return signal;
 }
 
 trading::execution::Order ExecutionServiceImpl::convertOrderRequest(
     const ::trading::proto::OrderRequest& proto_order) {
     
     trading::execution::Order order(proto_order.client_order_id());
     
     // Set basic fields
     order.setExchangeId(proto_order.exchange_id());
     order.setTradingPair(proto_order.trading_pair());
     
     // Convert side
     order.setSide(proto_order.side() == "buy" ? OrderSide::BUY : OrderSide::SELL);
     
     // Convert order type
     if (proto_order.order_type() == "market") {
         order.setType(OrderType::MARKET);
     } else if (proto_order.order_type() == "limit") {
         order.setType(OrderType::LIMIT);
     } else if (proto_order.order_type() == "stop") {
         order.setType(OrderType::STOP);
     } else if (proto_order.order_type() == "stop_limit") {
         order.setType(OrderType::STOP_LIMIT);
     } else if (proto_order.order_type() == "iceberg") {
         order.setType(OrderType::ICEBERG);
     } else {
         order.setType(OrderType::MARKET); // Default to market
     }
     
     // Set numeric fields
     order.setQuantity(proto_order.quantity());
     order.setPrice(proto_order.price());
     
     if (proto_order.has_stop_price()) {
         order.setStopPrice(proto_order.stop_price());
     }
     
     // Convert time in force
     if (proto_order.time_in_force() == "GTC") {
         order.setTimeInForce(TimeInForce::GTC);
     } else if (proto_order.time_in_force() == "IOC") {
         order.setTimeInForce(TimeInForce::IOC);
     } else if (proto_order.time_in_force() == "FOK") {
         order.setTimeInForce(TimeInForce::FOK);
     } else if (proto_order.time_in_force() == "GTD") {
         order.setTimeInForce(TimeInForce::GTD);
     } else {
         order.setTimeInForce(TimeInForce::GTC); // Default to GTC
     }
     
     // Set strategy info
     order.setStrategyId(proto_order.strategy_id());
     order.setSignalId(proto_order.signal_id());
     
     // Set extra parameters
     for (const auto& [key, value] : proto_order.parameters()) {
         order.setExtraParam(key, value);
     }
     
     return order;
 }
 
 ::trading::proto::ExecutionResponse ExecutionServiceImpl::createExecutionResponse(
     const trading::execution::ExecutionResponse& internal_response) {
     
     ::trading::proto::ExecutionResponse proto_response;
     
     // Set fields
     proto_response.set_client_order_id(internal_response.client_order_id);
     proto_response.set_exchange_order_id(internal_response.exchange_order_id);
     proto_response.set_success(internal_response.success);
     proto_response.set_message(internal_response.error_message);
     proto_response.set_status(internal_response.order_status);
     proto_response.set_timestamp(
         std::chrono::duration_cast<std::chrono::milliseconds>(
             internal_response.timestamp.time_since_epoch()
         ).count()
     );
     
     return proto_response;
 }
 
 ::trading::proto::CancelResponse ExecutionServiceImpl::createCancelResponse(
     const trading::execution::CancelResponse& internal_response) {
     
     ::trading::proto::CancelResponse proto_response;
     
     // Set fields
     proto_response.set_client_order_id(internal_response.client_order_id);
     proto_response.set_exchange_order_id(internal_response.exchange_order_id);
     proto_response.set_success(internal_response.success);
     proto_response.set_message(internal_response.error_message);
     proto_response.set_timestamp(
         std::chrono::duration_cast<std::chrono::milliseconds>(
             internal_response.timestamp.time_since_epoch()
         ).count()
     );
     
     return proto_response;
 }
 
 ::trading::proto::OrderStatusResponse ExecutionServiceImpl::createOrderStatusResponse(
     const trading::execution::OrderStatusResponse& internal_status) {
     
     ::trading::proto::OrderStatusResponse proto_response;
     
     // Set basic fields
     proto_response.set_found(internal_status.found);
     proto_response.set_error_message(internal_status.error_message);
     
     // Add order if found
     if (internal_status.found) {
         auto* order_proto = proto_response.mutable_order();
         *order_proto = createProtoOrder(internal_status.order);
     }
     
     return proto_response;
 }
 
 ::trading::proto::Order ExecutionServiceImpl::createProtoOrder(
     const trading::execution::Order& internal_order) {
     
     ::trading::proto::Order proto_order;
     
     // Set identification fields
     proto_order.set_client_order_id(internal_order.getClientOrderId());
     proto_order.set_exchange_order_id(internal_order.getExchangeOrderId());
     proto_order.set_exchange_id(internal_order.getExchangeId());
     proto_order.set_trading_pair(internal_order.getTradingPair());
     
     // Set side
     proto_order.set_side(internal_order.getSide() == OrderSide::BUY ? "buy" : "sell");
     
     // Set order type
     switch (internal_order.getType()) {
         case OrderType::MARKET:
             proto_order.set_order_type("market");
             break;
         case OrderType::LIMIT:
             proto_order.set_order_type("limit");
             break;
         case OrderType::STOP:
             proto_order.set_order_type("stop");
             break;
         case OrderType::STOP_LIMIT:
             proto_order.set_order_type("stop_limit");
             break;
         case OrderType::ICEBERG:
             proto_order.set_order_type("iceberg");
             break;
         default:
             proto_order.set_order_type("unknown");
             break;
     }
     
     // Set numeric fields
     proto_order.set_quantity(internal_order.getQuantity());
     proto_order.set_price(internal_order.getPrice());
     
     auto stop_price = internal_order.getStopPrice();
     if (stop_price) {
         proto_order.set_stop_price(*stop_price);
     }
     
     // Set time in force
     switch (internal_order.getTimeInForce()) {
         case TimeInForce::GTC:
             proto_order.set_time_in_force("GTC");
             break;
         case TimeInForce::IOC:
             proto_order.set_time_in_force("IOC");
             break;
         case TimeInForce::FOK:
             proto_order.set_time_in_force("FOK");
             break;
         case TimeInForce::GTD:
             proto_order.set_time_in_force("GTD");
             break;
     }
     
     // Set status
     switch (internal_order.getState()) {
         case OrderState::CREATED:
             proto_order.set_status("created");
             break;
         case OrderState::PENDING:
             proto_order.set_status("pending");
             break;
         case OrderState::PENDING_CANCEL:
             proto_order.set_status("pending_cancel");
             break;
         case OrderState::OPEN:
             proto_order.set_status("open");
             break;
         case OrderState::PARTIALLY_FILLED:
             proto_order.set_status("partially_filled");
             break;
         case OrderState::FILLED:
             proto_order.set_status("filled");
             break;
         case OrderState::CANCELED:
             proto_order.set_status("canceled");
             break;
         case OrderState::REJECTED:
             proto_order.set_status("rejected");
             break;
         case OrderState::EXPIRED:
             proto_order.set_status("expired");
             break;
     }
     
     // Set quantities
     proto_order.set_filled_quantity(internal_order.getFilledQuantity());
     proto_order.set_remaining_quantity(internal_order.getRemainingQuantity());
     proto_order.set_average_fill_price(internal_order.getAverageFillPrice());
     
     // Set timestamps
     proto_order.set_created_at(
         std::chrono::duration_cast<std::chrono::milliseconds>(
             internal_order.getCreatedTime().time_since_epoch()
         ).count()
     );
     
     proto_order.set_updated_at(
         std::chrono::duration_cast<std::chrono::milliseconds>(
             internal_order.getUpdatedTime().time_since_epoch()
         ).count()
     );
     
     // Set strategy info
     proto_order.set_strategy_id(internal_order.getStrategyId());
     proto_order.set_signal_id(internal_order.getSignalId());
     
     // Set fills
     for (const auto& fill : internal_order.getFills()) {
         auto* proto_fill = proto_order.add_fills();
         proto_fill->set_fill_id(fill.fill_id);
         proto_fill->set_quantity(fill.quantity);
         proto_fill->set_price(fill.price);
         proto_fill->set_fee(fill.fee);
         proto_fill->set_fee_currency(fill.fee_currency);
         proto_fill->set_timestamp(
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 fill.timestamp.time_since_epoch()
             ).count()
         );
     }
     
     // Set extra parameters
     for (const auto& [key, value] : internal_order.getExtraParams()) {
         (*proto_order.mutable_parameters())[key] = value;
     }
     
     return proto_order;
 }
 
 } // namespace grpc
 } // namespace execution
 } // namespace trading