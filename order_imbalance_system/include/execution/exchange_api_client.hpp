// exchange_api_client.hpp
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <future>
#include "../models/order.hpp"
#include "../models/execution_report.hpp"

namespace trading {
namespace execution {

enum class ExchangeRequestType {
    PlaceOrder,
    CancelOrder,
    GetOrderStatus,
    GetAccountBalance,
    GetMarketData
};

struct ExchangeCredentials {
    std::string apiKey;
    std::string secretKey;
    std::string passphrase;  // Some exchanges require this
    // Additional authentication data
};

class ExchangeApiClient {
public:
    virtual ~ExchangeApiClient() = default;
    
    // Order operations
    virtual std::future<ExecutionReport> placeOrder(const Order& order) = 0;
    virtual std::future<ExecutionReport> cancelOrder(const std::string& orderId, const std::string& symbol) = 0;
    virtual std::future<ExecutionReport> getOrderStatus(const std::string& orderId, const std::string& symbol) = 0;
    
    // Account operations
    virtual std::future<std::map<std::string, double>> getAccountBalances() = 0;
    
    // Market data operations
    virtual std::future<double> getCurrentPrice(const std::string& symbol) = 0;
    
    // Connection management
    virtual bool isConnected() const = 0;
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    
    // Exchange information
    virtual std::string getExchangeName() const = 0;
    virtual std::vector<std::string> getSupportedSymbols() const = 0;
    
    // Rate limit information
    virtual int getRemainingRequests(ExchangeRequestType requestType) const = 0;
    virtual std::chrono::milliseconds getResetTimeForRequestType(ExchangeRequestType requestType) const = 0;
};

} // namespace execution
} // namespace trading