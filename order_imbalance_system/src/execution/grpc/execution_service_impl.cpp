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
 
 ExecutionServiceImpl::ExecutionServiceImpl(std::shared_ptr<trading::execution::ExecutionService> execution_service)
     : execution_service_(std::move(execution_service)) {
     // Ensure service is not null
     if (!execution_service_) {
         throw std::invalid_argument("Execution service cannot be null");
     }
 }
 
 ::grpc::Status ExecutionServiceImpl::ExecuteSignal(
     ::grpc::ServerContext* context,
     const ::trading::execution::grpc::TradingSignalProto* request,
     ::trading::execution::grpc::ExecutionResponseProto* response) {
     
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
     const ::trading::execution::grpc::OrderProto* request,
     ::trading::execution::grpc::ExecutionResponseProto* response) {
     
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
     const ::trading::execution::grpc::CancelOrderRequestProto* request,
     ::trading::execution::grpc::CancelResponseProto* response) {
     
     // Determine which ID type is provided
     if (!request->client_order_id().empty()) {
         // Cancel by client order ID
         auto result = execution_service_->cancelOrder(request->client_order_id());
         
         // Set response fields
         response->set_client_order_id(result.client_order_id);
         response->set_exchange_order_id(result.exchange_order_id);
         response->set_success(result.success);
         response->set_error_message(result.error_message);
         response->set_timestamp(
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 result.timestamp.time_since_epoch()
             ).count()
         );
     } else {
         // Neither ID was provided
         response->set_success(false);
         response->set_error_message("client_order_id must be provided");
         response->set_timestamp(
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now().time_since_epoch()
             ).count()
         );
     }
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::CancelAllOrdersForSymbol(
     ::grpc::ServerContext* context,
     const ::trading::execution::grpc::CancelAllOrdersRequestProto* request,
     ::trading::execution::grpc::CancelAllOrdersResponseProto* response) {
 
     // Cancel all orders for the symbol
     auto results = execution_service_->cancelAllOrdersForSymbol(
         request->trading_pair(),
         request->exchange_id()
     );
     
     // Convert results to proto response
     for (const auto& result : results) {
         auto* cancel_response = response->add_responses();
         *cancel_response = createCancelResponse(result);
     }
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::GetOrderStatus(
     ::grpc::ServerContext* context,
     const ::trading::execution::grpc::OrderStatusRequestProto* request,
     ::trading::execution::grpc::OrderStatusResponseProto* response) {
     
     // Get the order status
     auto result = execution_service_->getOrderStatus(request->client_order_id());
     
     // Convert to proto response
     *response = createOrderStatusResponse(result);
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::GetActiveOrders(
     ::grpc::ServerContext* context,
     const ::trading::execution::grpc::ActiveOrdersRequestProto* request,
     ::trading::execution::grpc::ActiveOrdersResponseProto* response) {
     
     // Get all active orders
     auto active_orders = execution_service_->getActiveOrders();
     
     // Add orders to response
     for (const auto& order : active_orders) {
         auto* order_proto = response->add_orders();
         *order_proto = createProtoOrder(order);
     }
     
     return ::grpc::Status::OK;
 }
 
 ::grpc::Status ExecutionServiceImpl::GetHealthStatus(
     ::grpc::ServerContext* context,
     const ::trading::execution::grpc::HealthStatusRequestProto* request,
     ::trading::execution::grpc::HealthStatusResponseProto* response) {
     
     // Get health status from service
     std::string health_json = execution_service_->getHealthStatus();
     
     // Set response fields
     response->set_health_json(health_json);
     
     return ::grpc::Status::OK;
 }
 
 // Helper methods for type conversion
 trading::execution::TradingSignal ExecutionServiceImpl::convertTradingSignal(
    const ::trading::execution::grpc::TradingSignalProto& proto_signal) {
    
    trading::execution::TradingSignal signal;
    
    // Basic fields
    signal.signal_id = proto_signal.signal_id();
    signal.trading_pair = proto_signal.trading_pair();
    signal.strategy_id = proto_signal.strategy_id();
    signal.strategy_version = proto_signal.strategy_version();
    signal.target_size = proto_signal.target_size();
    signal.strength = proto_signal.strength();
    
    // Convert direction (enum to enum)
    signal.direction = (proto_signal.direction() == ::trading::execution::grpc::TradingSignalProto_OrderSide_BUY) 
        ? trading::execution::models::OrderSide::BUY 
        : trading::execution::models::OrderSide::SELL;
    
    // Convert TTL
    signal.signal_ttl = std::chrono::milliseconds(proto_signal.signal_ttl_ms());
    
    // Convert metrics
    for (const auto& entry : proto_signal.signal_metrics()) {
        signal.signal_metrics[entry.first] = entry.second;
    }
    
    return signal;
}

 trading::execution::models::Order ExecutionServiceImpl::convertOrderRequest(
    const ::trading::execution::grpc::OrderProto& proto_order) {
    
    trading::execution::models::Order order(proto_order.client_order_id());
    
    // Set basic fields
    order.setExchangeId(proto_order.exchange_id());
    order.setTradingPair(proto_order.trading_pair());
    
    // Convert side
    order.setSide(proto_order.side() == ::trading::execution::grpc::OrderProto_OrderSide_BUY 
        ? trading::execution::models::OrderSide::BUY 
        : trading::execution::models::OrderSide::SELL);
    
    // Convert order type
    switch (proto_order.type()) {
        case ::trading::execution::grpc::OrderProto_OrderType_MARKET:
            order.setType(trading::execution::models::OrderType::MARKET);
            break;
        case ::trading::execution::grpc::OrderProto_OrderType_LIMIT:
            order.setType(trading::execution::models::OrderType::LIMIT);
            break;
        case ::trading::execution::grpc::OrderProto_OrderType_STOP:
            order.setType(trading::execution::models::OrderType::STOP);
            break;
        case ::trading::execution::grpc::OrderProto_OrderType_STOP_LIMIT:
            order.setType(trading::execution::models::OrderType::STOP_LIMIT);
            break;
        case ::trading::execution::grpc::OrderProto_OrderType_ICEBERG:
            order.setType(trading::execution::models::OrderType::ICEBERG);
            break;
        default:
            order.setType(trading::execution::models::OrderType::MARKET); // Default to market
            break;
    }
    
    // Set numeric fields
    order.setQuantity(proto_order.quantity());
    order.setPrice(proto_order.price());
    
    if (proto_order.stop_price() != 0.0) {
        order.setStopPrice(proto_order.stop_price());
    }
    
    // Convert time in force
    switch (proto_order.time_in_force()) {
        case ::trading::execution::grpc::OrderProto_TimeInForce_GTC:
            order.setTimeInForce(trading::execution::models::TimeInForce::GTC);
            break;
        case ::trading::execution::grpc::OrderProto_TimeInForce_IOC:
            order.setTimeInForce(trading::execution::models::TimeInForce::IOC);
            break;
        case ::trading::execution::grpc::OrderProto_TimeInForce_FOK:
            order.setTimeInForce(trading::execution::models::TimeInForce::FOK);
            break;
        case ::trading::execution::grpc::OrderProto_TimeInForce_GTD:
            order.setTimeInForce(trading::execution::models::TimeInForce::GTD);
            break;
        default:
            order.setTimeInForce(trading::execution::models::TimeInForce::GTC); // Default to GTC
            break;
    }
    
    // Set strategy info
    order.setStrategyId(proto_order.strategy_id());
    order.setSignalId(proto_order.signal_id());
    
    // Set extra parameters
    for (const auto& entry : proto_order.extra_params()) {
        order.setExtraParam(entry.first, entry.second);
    }
    
    return order;
}
 ::trading::execution::grpc::ExecutionResponseProto ExecutionServiceImpl::createExecutionResponse(
     const trading::execution::ExecutionResponse& internal_response) {
     
     ::trading::execution::grpc::ExecutionResponseProto proto_response;
     
     // Set fields
     proto_response.set_client_order_id(internal_response.client_order_id);
     proto_response.set_exchange_order_id(internal_response.exchange_order_id);
     proto_response.set_success(internal_response.success);
     proto_response.set_error_message(internal_response.error_message);
     proto_response.set_order_status(internal_response.order_status);
     proto_response.set_timestamp(
         std::chrono::duration_cast<std::chrono::milliseconds>(
             internal_response.timestamp.time_since_epoch()
         ).count()
     );
     
     return proto_response;
 }
 
 ::trading::execution::grpc::CancelResponseProto ExecutionServiceImpl::createCancelResponse(
     const trading::execution::CancelResponse& internal_response) {
     
     ::trading::execution::grpc::CancelResponseProto proto_response;
     
     // Set fields
     proto_response.set_client_order_id(internal_response.client_order_id);
     proto_response.set_exchange_order_id(internal_response.exchange_order_id);
     proto_response.set_success(internal_response.success);
     proto_response.set_error_message(internal_response.error_message);
     proto_response.set_timestamp(
         std::chrono::duration_cast<std::chrono::milliseconds>(
             internal_response.timestamp.time_since_epoch()
         ).count()
     );
     
     return proto_response;
 }
 
 ::trading::execution::grpc::OrderStatusResponseProto ExecutionServiceImpl::createOrderStatusResponse(
     const trading::execution::OrderStatusResponse& internal_status) {
     
     ::trading::execution::grpc::OrderStatusResponseProto proto_response;
     
     // Set basic fields
     proto_response.set_found(internal_status.found);
     proto_response.set_error_message(internal_status.error_message);
     
     // Add order if found
     if (internal_status.found) {
         auto* order_proto = proto_response.mutable_order();
         *order_proto = createProtoOrder(*internal_status.order);
     }
     
     return proto_response;
 }
 ::trading::execution::grpc::OrderProto ExecutionServiceImpl::createProtoOrder(
    const trading::execution::models::Order& internal_order) {
    
    ::trading::execution::grpc::OrderProto proto_order;
    
    // Set identification fields
    proto_order.set_client_order_id(internal_order.getClientOrderId());
    proto_order.set_exchange_order_id(internal_order.getExchangeOrderId());
    proto_order.set_exchange_id(internal_order.getExchangeId());
    proto_order.set_trading_pair(internal_order.getTradingPair());
    
    // Set side
    proto_order.set_side(internal_order.getSide() == trading::execution::models::OrderSide::BUY ? 
        ::trading::execution::grpc::OrderProto_OrderSide_BUY : 
        ::trading::execution::grpc::OrderProto_OrderSide_SELL);
    
    // Set order type
    switch (internal_order.getType()) {
        case trading::execution::models::OrderType::MARKET:
            proto_order.set_type(::trading::execution::grpc::OrderProto_OrderType_MARKET);
            break;
        case trading::execution::models::OrderType::LIMIT:
            proto_order.set_type(::trading::execution::grpc::OrderProto_OrderType_LIMIT);
            break;
        case trading::execution::models::OrderType::STOP:
            proto_order.set_type(::trading::execution::grpc::OrderProto_OrderType_STOP);
            break;
        case trading::execution::models::OrderType::STOP_LIMIT:
            proto_order.set_type(::trading::execution::grpc::OrderProto_OrderType_STOP_LIMIT);
            break;
        case trading::execution::models::OrderType::ICEBERG:
            proto_order.set_type(::trading::execution::grpc::OrderProto_OrderType_ICEBERG);
            break;
        default:
            proto_order.set_type(::trading::execution::grpc::OrderProto_OrderType_MARKET); // Default to market
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
        case trading::execution::models::TimeInForce::GTC:
            proto_order.set_time_in_force(::trading::execution::grpc::OrderProto_TimeInForce_GTC);
            break;
        case trading::execution::models::TimeInForce::IOC:
            proto_order.set_time_in_force(::trading::execution::grpc::OrderProto_TimeInForce_IOC);
            break;
        case trading::execution::models::TimeInForce::FOK:
            proto_order.set_time_in_force(::trading::execution::grpc::OrderProto_TimeInForce_FOK);
            break;
        case trading::execution::models::TimeInForce::GTD:
            proto_order.set_time_in_force(::trading::execution::grpc::OrderProto_TimeInForce_GTD);
            break;
        default:
            proto_order.set_time_in_force(::trading::execution::grpc::OrderProto_TimeInForce_GTC); // Default to GTC
            break;
    }
    
    // Set status
    switch (internal_order.getState()) {
        case trading::execution::models::OrderState::CREATED:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::CREATED);
            break;
        case trading::execution::models::OrderState::PENDING:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::PENDING);
            break;
        case trading::execution::models::OrderState::PENDING_CANCEL:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::PENDING_CANCEL);
            break;
        case trading::execution::models::OrderState::OPEN:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::OPEN);
            break;
        case trading::execution::models::OrderState::PARTIALLY_FILLED:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::PARTIALLY_FILLED);
            break;
        case trading::execution::models::OrderState::FILLED:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::FILLED);
            break;
        case trading::execution::models::OrderState::CANCELED:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::CANCELED);
            break;
        case trading::execution::models::OrderState::REJECTED:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::REJECTED);
            break;
        case trading::execution::models::OrderState::EXPIRED:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::EXPIRED);
            break;
        default:
            proto_order.set_state(::trading::execution::grpc::OrderStateProto::CREATED);
            break;
    }
    
    // Set quantities
    proto_order.set_filled_quantity(internal_order.getFilledQuantity());
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
        proto_fill->set_order_id(fill.orderId);
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
    for (const auto& entry : internal_order.getExtraParams()) {
        (*proto_order.mutable_extra_params())[entry.first] = entry.second;
    }
    
    return proto_order;
}
//  ::trading::execution::grpc::OrderProto ExecutionServiceImpl::createProtoOrder(
//      const trading::execution::models::Order& internal_order) {
     
//      ::trading::execution::grpc::OrderProto proto_order;
     
//      // Set identification fields
//      proto_order.set_client_order_id(internal_order.getClientOrderId());
//      proto_order.set_exchange_order_id(internal_order.getExchangeOrderId());
//      proto_order.set_exchange_id(internal_order.getExchangeId());
//      proto_order.set_trading_pair(internal_order.getTradingPair());
     
//      // Set side
//      proto_order.set_side(internal_order.getSide() == trading::execution::models::OrderSide::BUY  ? OrderProto::BUY : OrderProto::SELL);
     
//      // Set order type
//      switch (internal_order.getType()) {
//          case OrderType::MARKET:
//              proto_order.set_type(OrderProto::MARKET);
//              break;
//          case OrderType::LIMIT:
//              proto_order.set_type(OrderProto::LIMIT);
//              break;
//          case OrderType::STOP:
//              proto_order.set_type(OrderProto::STOP);
//              break;
//          case OrderType::STOP_LIMIT:
//              proto_order.set_type(OrderProto::STOP_LIMIT);
//              break;
//          case OrderType::ICEBERG:
//              proto_order.set_type(OrderProto::ICEBERG);
//              break;
//          default:
//              proto_order.set_type(OrderProto::MARKET); // Default to market
//              break;
//      }
     
//      // Set numeric fields
//      proto_order.set_quantity(internal_order.getQuantity());
//      proto_order.set_price(internal_order.getPrice());
     
//      auto stop_price = internal_order.getStopPrice();
//      if (stop_price) {
//          proto_order.set_stop_price(*stop_price);
//      }
     
//      // Set time in force
//      switch (internal_order.getTimeInForce()) {
//          case TimeInForce::GTC:
//              proto_order.set_time_in_force(OrderProto::GTC);
//              break;
//          case TimeInForce::IOC:
//              proto_order.set_time_in_force(OrderProto::IOC);
//              break;
//          case TimeInForce::FOK:
//              proto_order.set_time_in_force(OrderProto::FOK);
//              break;
//          case TimeInForce::GTD:
//              proto_order.set_time_in_force(OrderProto::GTD);
//              break;
//          default:
//              proto_order.set_time_in_force(OrderProto::GTC); // Default to GTC
//              break;
//      }
     
//      // Set status
//      switch (internal_order.getState()) {
//          case OrderState::CREATED:
//              proto_order.set_state(OrderStateProto::CREATED);
//              break;
//          case OrderState::PENDING:
//              proto_order.set_state(OrderStateProto::PENDING);
//              break;
//          case OrderState::PENDING_CANCEL:
//              proto_order.set_state(OrderStateProto::PENDING_CANCEL);
//              break;
//          case OrderState::OPEN:
//              proto_order.set_state(OrderStateProto::OPEN);
//              break;
//          case OrderState::PARTIALLY_FILLED:
//              proto_order.set_state(OrderStateProto::PARTIALLY_FILLED);
//              break;
//          case OrderState::FILLED:
//              proto_order.set_state(OrderStateProto::FILLED);
//              break;
//          case OrderState::CANCELED:
//              proto_order.set_state(OrderStateProto::CANCELED);
//              break;
//          case OrderState::REJECTED:
//              proto_order.set_state(OrderStateProto::REJECTED);
//              break;
//          case OrderState::EXPIRED:
//              proto_order.set_state(OrderStateProto::EXPIRED);
//              break;
//          default:
//              proto_order.set_state(OrderStateProto::CREATED);
//              break;
//      }
     
//      // Set quantities
//      proto_order.set_filled_quantity(internal_order.getFilledQuantity());
//      proto_order.set_average_fill_price(internal_order.getAverageFillPrice());
     
//      // Set timestamps
//      proto_order.set_created_at(
//          std::chrono::duration_cast<std::chrono::milliseconds>(
//              internal_order.getCreatedTime().time_since_epoch()
//          ).count()
//      );
     
//      proto_order.set_updated_at(
//          std::chrono::duration_cast<std::chrono::milliseconds>(
//              internal_order.getUpdatedTime().time_since_epoch()
//          ).count()
//      );
     
//      // Set strategy info
//      proto_order.set_strategy_id(internal_order.getStrategyId());
//      proto_order.set_signal_id(internal_order.getSignalId());
     
//      // Set fills
//      for (const auto& fill : internal_order.getFills()) {
//          auto* proto_fill = proto_order.add_fills();
//          proto_fill->set_fill_id(fill.fill_id);
//          proto_fill->set_order_id(fill.order_id);
//          proto_fill->set_quantity(fill.quantity);
//          proto_fill->set_price(fill.price);
//          proto_fill->set_fee(fill.fee);
//          proto_fill->set_fee_currency(fill.fee_currency);
//          proto_fill->set_timestamp(
//              std::chrono::duration_cast<std::chrono::milliseconds>(
//                  fill.timestamp.time_since_epoch()
//              ).count()
//          );
//      }
     
//      // Set extra parameters
//      for (const auto& entry : internal_order.getExtraParams()) {
//          (*proto_order.mutable_extra_params())[entry.first] = entry.second;
//      }
     
//      return proto_order;
//  }
 
 } // namespace grpc
 
 // ExecutionServiceServer implementation
 ExecutionServiceServer::ExecutionServiceServer(
     std::shared_ptr<ExecutionService> execution_service,
     const std::string& server_address)
     : service_(std::move(execution_service)), server_address_(server_address) {
 }
 
 void ExecutionServiceServer::start() {
     ::grpc::ServerBuilder builder;
     builder.AddListeningPort(server_address_, ::grpc::InsecureServerCredentials());
     builder.RegisterService(&service_);
     server_ = builder.BuildAndStart();
     std::cout << "Server listening on " << server_address_ << std::endl;
 }
 
 void ExecutionServiceServer::shutdown() {
     if (server_) {
         server_->Shutdown();
     }
 }
 
 void ExecutionServiceServer::waitForShutdown() {
     if (server_) {
         server_->Wait();
     }
 }
 
 } // namespace execution
 } // namespace trading