// exchanges/binance_api_client.hpp
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "../exchange_api_client.hpp"
#include "../rate_limit_controller.hpp"

namespace trading {
namespace execution {

class BinanceApiClient : public ExchangeApiClient {
public:
    BinanceApiClient(const ExchangeCredentials& credentials, 
                    std::shared_ptr<RateLimitController> rateLimitController);
    ~BinanceApiClient() override;
    
    // Implement ExchangeApiClient interface
    std::future<ExecutionReport> placeOrder(const Order& order) override;
    std::future<ExecutionReport> cancelOrder(const std::string& orderId, const std::string& symbol) override;
    std::future<ExecutionReport> getOrderStatus(const std::string& orderId, const std::string& symbol) override;
    std::future<std::map<std::string, double>> getAccountBalances() override;
    std::future<double> getCurrentPrice(const std::string& symbol) override;
    bool isConnected() const override;
    void connect() override;
    void disconnect() override;
    std::string getExchangeName() const override;
    std::vector<std::string> getSupportedSymbols() const override;
    int getRemainingRequests(ExchangeRequestType requestType) const override;
    std::chrono::milliseconds getResetTimeForRequestType(ExchangeRequestType requestType) const override;

private:
    struct BinanceImpl;
    std::unique_ptr<BinanceImpl> pImpl_;
    std::shared_ptr<RateLimitController> rateLimitController_;
    
    // Binance-specific methods
    std::string generateSignature(const std::string& queryString) const;
    std::string makeApiRequest(const std::string& endpoint, const std::string& method, 
                               const std::map<std::string, std::string>& params, bool isPrivate);
    
    // Helper methods for converting between internal and Binance-specific formats
    std::string translateOrderType(OrderType type) const;
    std::string translateTimeInForce(TimeInForce tif) const;
    std::string translateOrderSide(OrderSide side) const;
    ExecutionReport parseOrderResponse(const std::string& response) const;
};

} // namespace execution
} // namespace trading