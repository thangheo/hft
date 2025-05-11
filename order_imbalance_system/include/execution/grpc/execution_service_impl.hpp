// execution_service_impl.hpp
#pragma once

#include "execution/execution_service.hpp"
#include "execution_service.grpc.pb.h"
#include <memory>
#include <grpcpp/grpcpp.h>

namespace trading {
namespace execution {
namespace grpc {

// Implementation of the gRPC service
class ExecutionServiceImpl final : public ::trading::execution::grpc::ExecutionService::Service {
public: // Add the public specifier
    ExecutionServiceImpl(std::shared_ptr<trading::execution::ExecutionService> execution_service);
    
    ::grpc::Status ExecuteSignal(
        ::grpc::ServerContext* context, 
        const ::trading::execution::grpc::TradingSignalProto* request,
        ::trading::execution::grpc::ExecutionResponseProto* response) override;
    
    ::grpc::Status PlaceOrder(
        ::grpc::ServerContext* context, 
        const ::trading::execution::grpc::OrderProto* request,
        ::trading::execution::grpc::ExecutionResponseProto* response) override;
    
    ::grpc::Status CancelOrder(
        ::grpc::ServerContext* context, 
        const ::trading::execution::grpc::CancelOrderRequestProto* request,
        ::trading::execution::grpc::CancelResponseProto* response) override;
    
    ::grpc::Status CancelAllOrdersForSymbol(
        ::grpc::ServerContext* context, 
        const ::trading::execution::grpc::CancelAllOrdersRequestProto* request,
        ::trading::execution::grpc::CancelAllOrdersResponseProto* response) override;
    
    ::grpc::Status GetOrderStatus(
        ::grpc::ServerContext* context, 
        const ::trading::execution::grpc::OrderStatusRequestProto* request,
        ::trading::execution::grpc::OrderStatusResponseProto* response) override;
    
    ::grpc::Status GetActiveOrders(
        ::grpc::ServerContext* context, 
        const ::trading::execution::grpc::ActiveOrdersRequestProto* request,
        ::trading::execution::grpc::ActiveOrdersResponseProto* response) override;
    
    ::grpc::Status GetHealthStatus(
        ::grpc::ServerContext* context, 
        const ::trading::execution::grpc::HealthStatusRequestProto* request,
        ::trading::execution::grpc::HealthStatusResponseProto* response) override;

private:
    std::shared_ptr<trading::execution::ExecutionService> execution_service_;
    
    // Helper conversion methods
    trading::execution::TradingSignal convertTradingSignal(
        const ::trading::execution::grpc::TradingSignalProto& proto);
    
    trading::execution::models::Order convertOrderRequest(
        const ::trading::execution::grpc::OrderProto& proto);
    
    ::trading::execution::grpc::OrderProto createProtoOrder(
        const trading::execution::models::Order& order);
    
    ::trading::execution::grpc::ExecutionResponseProto createExecutionResponse(
        const trading::execution::ExecutionResponse& response);
    
    ::trading::execution::grpc::CancelResponseProto createCancelResponse(
        const trading::execution::CancelResponse& response);
    
    ::trading::execution::grpc::OrderStatusResponseProto createOrderStatusResponse(
        const trading::execution::OrderStatusResponse& response);
};

} // namespace grpc

// Server class to run the gRPC service - This stays in the execution namespace
class ExecutionServiceServer {
public:
    ExecutionServiceServer(
        std::shared_ptr<trading::execution::ExecutionService> execution_service,
        const std::string& server_address);
    
    void start();
    void shutdown();
    void waitForShutdown();

private:
    std::unique_ptr<::grpc::Server> server_;
    trading::execution::grpc::ExecutionServiceImpl service_;
    std::string server_address_;
};

} // namespace execution
} // namespace trading