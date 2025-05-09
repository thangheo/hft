// api_client_manager.hpp
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "exchange_api_client.hpp"
#include "authentication_service.hpp"
#include "rate_limit_controller.hpp"
#include "exchanges/binance_api_client.hpp"
#include "exchanges/ftx_api_client.hpp"
namespace trading {
namespace execution {

class ApiClientManager {
public:
    ApiClientManager(std::shared_ptr<AuthenticationService> authService);
    ~ApiClientManager();
    
    // Get a client for a specific exchange
    std::shared_ptr<ExchangeApiClient> getClient(const std::string& exchangeId);
    // 
    // Check if a client for a specific exchange exists
    bool hasClient(const std::string& exchangeId) const;
    
    // Get all available exchange clients
    std::vector<std::string> getAvailableExchanges() const;
    //getHealthStatus
    std::string getHealthStatus() const;
    // Get exchange statuses
    std::unordered_map<std::string, std::string> getExchangeStatuses() const;
    // Health check for all exchange connections
    std::unordered_map<std::string, bool> checkConnections() const;
    
    // Add a new exchange client
    void addClient(const std::string& exchangeId, std::shared_ptr<ExchangeApiClient> client);
    
    // Remove an exchange client
    void removeClient(const std::string& exchangeId);
    
    // Get rate limit status for an exchange
    RateLimitStatus getRateLimitStatus(const std::string& exchangeId, ExchangeRequestType requestType) const;
    bool initialize() ;
    void shutdown() ;
    
    
    
private:
    std::unordered_map<std::string, std::shared_ptr<ExchangeApiClient>> clients_;
    std::shared_ptr<AuthenticationService> authService_;
    std::unordered_map<std::string, std::shared_ptr<RateLimitController>> rateLimitControllers_;
    mutable std::mutex mutex_;
    
    // Create a new exchange client based on exchange ID
    std::shared_ptr<ExchangeApiClient> createClient(const std::string& exchangeId);
};

} // namespace execution
} // namespace trading