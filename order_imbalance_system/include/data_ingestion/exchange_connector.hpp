// include/data_ingestion/exchange_connector.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>

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
    virtual bool isConnected() const = 0;
    
    // Get exchange identifier
    std::string getExchangeId() const { return exchange_id_; }
    
protected:
    std::string exchange_id_;
    MarketDataCallback data_callback_;
    bool connected_;
};

} // namespace data_ingestion
} // namespace trading
