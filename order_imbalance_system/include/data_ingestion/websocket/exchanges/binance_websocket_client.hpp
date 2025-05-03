// include/data_ingestion/websocket/exchanges/binance_websocket_client.hpp
#pragma once

#include "data_ingestion/websocket/client_base.hpp"
#include "data_ingestion/websocket/exchanges/exchange_connector.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace trading {
namespace data_ingestion {

using json = nlohmann::json;

class BinanceWebSocketClient : public WebSocketClientBase {
public:
    BinanceWebSocketClient(const std::string& host,
                          const std::string& port,
                          MarketDataCallback callback);
    ~BinanceWebSocketClient() override;
    
    // Subscription management
    bool subscribe(const std::vector<std::string>& streams);
    bool unsubscribe(const std::vector<std::string>& streams);
    
    // Implementation of WebSocketClientBase
    bool isConnected() const override;

private:
    // WebSocketClientBase implementation
    void onOpen() override;
    void onMessage(const std::string& message) override;
    void onClose(int code, const std::string& reason) override;
    void onError(const std::string& error) override;
    void onReconnect() override;
    
    // Binance-specific message handlers
    void handleStreamMessage(const json& data);
    void handleResponse(const json& data);
    void processOrderBookData(const std::string& symbol, const std::string& event_type, const json& data);
    void processTrade(const std::string& symbol, const json& data);
    void processTicker(const std::string& symbol, const json& data);
    void processBookTicker(const std::string& symbol, const json& data);
    
    // Subscription management
    bool sendSubscriptionRequest(const std::vector<std::string>& streams, bool subscribe);
    
    // Data callback
    MarketDataCallback data_callback_;
    
    // Subscription tracking
    std::mutex subscription_mutex_;
    std::vector<std::string> subscribed_streams_;
    int subscription_id_counter_ = 1;
    
    // Helpers
    std::string extractSymbolFromStream(const std::string& stream) const;
    std::string extractEventTypeFromStream(const std::string& stream) const;
};

} // namespace data_ingestion
} // namespace trading