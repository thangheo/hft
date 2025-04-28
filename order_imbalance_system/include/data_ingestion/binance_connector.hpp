// include/data_ingestion/binance_connector.hpp
#pragma once

#include "exchange_connector.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace trading {
namespace data_ingestion {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

class BinanceConnector : public ExchangeConnector {
public:
    BinanceConnector(const std::string& host, 
                     const std::string& port,
                     MarketDataCallback callback);
    ~BinanceConnector() override;
    
    bool connect() override;
    void disconnect() override;
    bool subscribe(const SubscriptionRequest& request) override;
    bool isConnected() const override { return connected_; }
    
private:
    // Configuration
    std::string host_;
    std::string port_;
    
    // Connection handling
    net::io_context io_context_;
    ssl::context ssl_context_{ssl::context::tlsv12_client};
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> websocket_ssl_stream_;
    std::thread io_thread_;
    std::atomic<bool> keep_running_;
    
    // Subscription management
    int subscription_id_counter_ = 1;
    std::vector<std::string> subscribed_streams_;
    
    // Internal methods
    void runIoService();
    void onMessage(const std::string& msg);
    void subscribeToStream(const std::string& stream);
    
    // Message handlers
    void handleOrderBookUpdate(const json& data, const std::string& symbol);
    void handleTradeUpdate(const json& data, const std::string& symbol);
    void handleTickerUpdate(const json& data, const std::string& symbol);
    void handleBookTickerUpdate(const json& data, const std::string& symbol);
};

} // namespace data_ingestion
} // namespace trading