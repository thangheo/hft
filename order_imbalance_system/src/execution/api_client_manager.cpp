// api_client_manager.cpp
#include "execution/api_client_manager.hpp"
#include "execution/exchanges/binance_api_client.hpp"
#include "execution/exchanges/ftx_api_client.hpp"
#include <stdexcept>
#include <iostream>

namespace trading {
namespace execution {

ApiClientManager::ApiClientManager(std::shared_ptr<AuthenticationService> authService)
    : authService_(std::move(authService)) {
    if (!authService_) {
        throw std::invalid_argument("Authentication service cannot be null");
    }
}

ApiClientManager::~ApiClientManager() = default;

std::shared_ptr<ExchangeApiClient> ApiClientManager::getClient(const std::string& exchangeId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if client already exists
    auto it = clients_.find(exchangeId);
    if (it != clients_.end()) {
        return it->second;
    }
    
    // Create a new client if it doesn't exist
    auto client = createClient(exchangeId);
    if (client) {
        clients_[exchangeId] = client;
        
        // Create a rate limit controller for this exchange if it doesn't exist
        if (rateLimitControllers_.find(exchangeId) == rateLimitControllers_.end()) {
            rateLimitControllers_[exchangeId] = std::make_shared<RateLimitController>(exchangeId);
        }
        
        return client;
    }
    
    throw std::runtime_error("Failed to create client for exchange: " + exchangeId);
}

bool ApiClientManager::hasClient(const std::string& exchangeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return clients_.find(exchangeId) != clients_.end();
}

std::vector<std::string> ApiClientManager::getAvailableExchanges() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> exchanges;
    exchanges.reserve(clients_.size());
    
    for (const auto& pair : clients_) {
        exchanges.push_back(pair.first);
    }
    
    return exchanges;
}

std::unordered_map<std::string, bool> ApiClientManager::checkConnections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, bool> statuses;
    
    for (const auto& pair : clients_) {
        const auto& exchangeId = pair.first;
        const auto& client = pair.second;
        try {
            bool isConnected = client->checkConnection();
            statuses[exchangeId] = isConnected;
        } catch (const std::exception& e) {
            std::cerr << "Error checking connection for " << exchangeId 
                      << ": " << e.what() << std::endl;
            statuses[exchangeId] = false;
        }
    }
    
    return statuses;
}

void ApiClientManager::addClient(const std::string& exchangeId, 
                                std::shared_ptr<ExchangeApiClient> client) {
    if (!client) {
        throw std::invalid_argument("Client cannot be null");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    clients_[exchangeId] = std::move(client);
    
    // Create a rate limit controller if it doesn't exist
    if (rateLimitControllers_.find(exchangeId) == rateLimitControllers_.end()) {
        rateLimitControllers_[exchangeId] = std::make_shared<RateLimitController>(exchangeId);
    }
}

void ApiClientManager::removeClient(const std::string& exchangeId) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.erase(exchangeId);
    rateLimitControllers_.erase(exchangeId);
}

RateLimitStatus ApiClientManager::getRateLimitStatus(const std::string& exchangeId, 
                                                  ExchangeRequestType requestType) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = rateLimitControllers_.find(exchangeId);
    if (it == rateLimitControllers_.end()) {
        throw std::runtime_error("No rate limit controller found for exchange: " + exchangeId);
    }
    
    return it->second->getStatus(requestType);
}

std::shared_ptr<ExchangeApiClient> ApiClientManager::createClient(const std::string& exchangeId) {
    // Get credentials from the authentication service
    auto credentials = authService_->getCredentials(exchangeId);
    
    // Create the appropriate client based on the exchange ID
    if (exchangeId == "binance") {
        return std::make_shared<BinanceApiClient>(credentials);
    } else if (exchangeId == "ftx") {
        return std::make_shared<FtxApiClient>(credentials);
    } else {
        throw std::runtime_error("Unsupported exchange: " + exchangeId);
    }
}

} // namespace execution
} // namespace trading