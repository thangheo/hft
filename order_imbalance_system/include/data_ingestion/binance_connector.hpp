// include/data_ingestion/binance_connector.hpp
#pragma once

#include "exchange_connector.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <nlohmann/json.hpp>
#include <thread>

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
    void runIoService();
    void onMessage(const std::string& msg);
    void handleOrderBookUpdate(const json& data, const std::string& symbol);
    
    std::string host_;
    std::string port_;
    
    net::io_context io_context_;
    ssl::context ssl_context_{ssl::context::tlsv12_client};
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
    std::thread io_thread_;
    std::map<std::string, int32_t> subscribed_pairs_;
};

} // namespace data_ingestion
} // namespace trading
