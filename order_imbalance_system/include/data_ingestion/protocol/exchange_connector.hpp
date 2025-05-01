// include/data_ingestion/exchange_connector.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <chrono>
#include <thread>

namespace trading {
namespace data_ingestion {

enum class DataType {
    TRADES,
    ORDER_BOOK,
    TICKER
};

struct SubscriptionRequest {
    std::string exchange_id;
    std::vector<std::string> trading_pairs;
    std::vector<DataType> data_types;
    int32_t depth;  // For order book subscriptions
};

struct MarketDataUpdate {
    std::string exchange_id;
    std::string trading_pair;
    std::string update_type;
    int64_t exchange_timestamp;
    int64_t received_timestamp;
    std::vector<uint8_t> data;  // Exchange-specific data format
};

// Callback type for market data updates
using MarketDataCallback = std::function<void(const MarketDataUpdate&)>;

class ExchangeConnector {
public:
    ExchangeConnector(const std::string& exchange_id, 
                      MarketDataCallback callback);
    virtual ~ExchangeConnector() = default;
    
    // Connect to the exchange WebSocket
    virtual bool connect() = 0;
    
    // Disconnect from the exchange WebSocket
    virtual void disconnect() = 0;
    
    // Subscribe to market data
    virtual bool subscribe(const SubscriptionRequest& request) = 0;
    
    // Get exchange status
    virtual bool isConnected() const { return connected_; }
    
    // Get exchange identifier
    std::string getExchangeId() const { return exchange_id_; }
    
    // Get the last error
    std::string getLastError() const;
    
    // Set reconnection parameters
    void setReconnectionParameters(int attempt_limit, int delay_ms);
    
    // Utility functions for data type conversion
    static DataType stringToDataType(const std::string& type_str);
    static std::string dataTypeToString(DataType type);
    
protected:
    std::string exchange_id_;
    MarketDataCallback data_callback_;
    bool connected_;
    
    // Error handling
    mutable std::mutex error_mutex_;
    std::string last_error_;
    std::chrono::system_clock::time_point last_error_time_;
    void reportError(const std::string& error_msg, bool is_fatal = false);
    
    // Reconnection logic
    int reconnect_attempt_limit_;
    int current_reconnect_attempts_;
    int reconnect_delay_ms_;
    bool tryReconnect();
    
    // Active subscriptions management
    std::vector<SubscriptionRequest> active_subscriptions_;
    void addSubscriptionRequest(const SubscriptionRequest& request);
    void clearSubscriptionRequests();
    std::vector<SubscriptionRequest> getActiveSubscriptions() const;
};

} // namespace data_ingestion
} // namespace trading