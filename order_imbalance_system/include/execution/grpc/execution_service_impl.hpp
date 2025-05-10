/**
 * @file execution_service_impl.hpp
 * @brief gRPC implementation of the execution service interface
 * 
 * @details 
 * Implements the gRPC service interface for the execution engine,
 * translating between gRPC messages and internal data structures.
 * 
 * Created on: May 5, 2025
 */
#pragma once

#include "execution/execution_service.hpp"
#include "execution/execution_types.hpp"
#include "execution_service.grpc.pb.h"  // Generated from protobuf

#include <memory>
#include <grpcpp/grpcpp.h>

namespace trading {
namespace execution {
namespace grpc {

/**
 * @brief gRPC implementation of the execution service
 */
class ExecutionServiceImpl final : public trading::proto::ExecutionService::Service {
public:
    /**
     * @brief Constructor
     * 
     * @param execution_service Reference to the core execution service
     */
    explicit ExecutionServiceImpl(std::shared_ptr<ExecutionService> execution_service);
    
    /**
     * @brief Destructor
     */
    ~ExecutionServiceImpl() override = default;
    
    /**
     * @brief Execute a trading signal (gRPC method)
     * 
     * @param context The gRPC context
     * @param request The execution request
     * @param response The execution response
     * @return gRPC status
     */
    ::grpc::Status ExecuteSignal(
        ::grpc::ServerContext* context,
        const ::trading::proto::TradingSignal* request,
        ::trading::proto::ExecutionResponse* response) override;
    
    /**
     * @brief Place an order directly (gRPC method)
     * 
     * @param context The gRPC context
     * @param request The order request
     * @param response The execution response
     * @return gRPC status
     */
    ::grpc::Status PlaceOrder(
        ::grpc::ServerContext* context,
        const ::trading::proto::OrderRequest* request,
        ::trading::proto::ExecutionResponse* response) override;
    
    /**
     * @brief Cancel an order (gRPC method)
     * 
     * @param context The gRPC context
     * @param request The cancel request
     * @param response The cancel response
     * @return gRPC status
     */
    ::grpc::Status CancelOrder(
        ::grpc::ServerContext* context,
        const ::trading::proto::CancelRequest* request,
        ::trading::proto::CancelResponse* response) override;
    
    /**
     * @brief Get order status (gRPC method)
     * 
     * @param context The gRPC context
     * @param request The status request
     * @param response The order status response
     * @return gRPC status
     */
    ::grpc::Status GetOrderStatus(
        ::grpc::ServerContext* context,
        const ::trading::proto::StatusRequest* request,
        ::trading::proto::OrderStatusResponse* response) override;
    
    /**
     * @brief Get active orders (gRPC method)
     * 
     * @param context The gRPC context
     * @param request The active orders request
     * @param response The active orders response
     * @return gRPC status
     */
    ::grpc::Status GetActiveOrders(
        ::grpc::ServerContext* context,
        const ::trading::proto::ActiveOrdersRequest* request,
        ::trading::proto::ActiveOrdersResponse* response) override;
    
    /**
     * @brief Get health status (gRPC method)
     * 
     * @param context The gRPC context
     * @param request The health request
     * @param response The health response
     * @return gRPC status
     */
    ::grpc::Status GetHealthStatus(
        ::grpc::ServerContext* context,
        const ::trading::proto::HealthRequest* request,
        ::trading::proto::HealthResponse* response) override;
    
private:
    // Reference to the core execution service
    std::shared_ptr<ExecutionService> execution_service_;
    
    // Helper methods for converting between gRPC and internal types
    trading::execution::TradingSignal convertTradingSignal(
        const ::trading::proto::TradingSignal& proto_signal);
    
    trading::execution::Order convertOrderRequest(
        const ::trading::proto::OrderRequest& proto_order);
    
    ::trading::proto::ExecutionResponse createExecutionResponse(
        const trading::execution::ExecutionResponse& internal_response);
    
    ::trading::proto::CancelResponse createCancelResponse(
        const trading::execution::CancelResponse& internal_response);
    
    ::trading::proto::OrderStatusResponse createOrderStatusResponse(
        const trading::execution::OrderStatusResponse& internal_status);
    
    ::trading::proto::Order createProtoOrder(
        const trading::execution::Order& internal_order);
};

} // namespace grpc
} // namespace execution
} // namespace trading