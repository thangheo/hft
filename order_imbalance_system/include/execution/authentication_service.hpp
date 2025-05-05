// authentication_service.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include "exchange_api_client.hpp"

namespace trading {
namespace execution {

class AuthenticationService {
public:
    AuthenticationService();
    ~AuthenticationService();
    
    // Get credentials for a specific exchange
    std::optional<ExchangeCredentials> getCredentials(const std::string& exchangeId) const;
    
    // Add or update credentials for an exchange
    void setCredentials(const std::string& exchangeId, const ExchangeCredentials& credentials);
    
    // Remove credentials for an exchange
    void removeCredentials(const std::string& exchangeId);
    
    // Check if credentials exist for an exchange
    bool hasCredentials(const std::string& exchangeId) const;
    
    // Validate that credentials are correct and working
    bool validateCredentials(const std::string& exchangeId, const ExchangeCredentials& credentials);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ExchangeCredentials> credentials_;
    
    // Encrypt credentials for storage
    ExchangeCredentials encryptCredentials(const ExchangeCredentials& credentials) const;
    
    // Decrypt stored credentials
    ExchangeCredentials decryptCredentials(const ExchangeCredentials& encryptedCredentials) const;
};

} // namespace execution
} // namespace trading