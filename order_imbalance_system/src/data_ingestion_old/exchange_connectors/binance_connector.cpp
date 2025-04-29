// src/data_ingestion/binance_connector.cpp
#include "data_ingestion/binance_connector.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace trading {
namespace data_ingestion {

BinanceConnector::BinanceConnector(const std::string& host, 
                                   const std::string& port,
                                   MarketDataCallback callback)
    : ExchangeConnector("binance", std::move(callback)),
      host_(host),
      port_(port),
      keep_running_(false) {
    
    // Initialize SSL context
    ssl_context_.set_default_verify_paths();
    ssl_context_.set_verify_mode(ssl::verify_peer);
}

BinanceConnector::~BinanceConnector() {
    disconnect();
}

bool BinanceConnector::connect() {
    try {
        if (connected_) {
            disconnect();
        }
        
        std::cout << "[Binance] Connecting to " << host_ << ":" << port_ << "..." << std::endl;
        
        // Create the I/O context
        io_context_.restart();
        
        // Resolve host
        tcp::resolver resolver(io_context_);
        auto const results = resolver.resolve(host_, port_);
        
        // Create SSL stream
        tcp::socket socket(io_context_);
        websocket_ssl_stream_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(
            std::move(socket), ssl_context_);
            
        // Set timeouts
        beast::get_lowest_layer(*websocket_ssl_stream_).expires_after(std::chrono::seconds(30));
        
        // Connect to the endpoint
        boost::asio::connect(beast::get_lowest_layer(*websocket_ssl_stream_), results);
        
        // Perform SSL handshake
        beast::get_lowest_layer(*websocket_ssl_stream_).expires_after(std::chrono::seconds(30));
        websocket_ssl_stream_->next_layer().handshake(ssl::stream_base::client);
        
        // Set websocket options
        websocket_ssl_stream_->set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));
        
        // Perform the WebSocket handshake
        beast::get_lowest_layer(*websocket_ssl_stream_).expires_never();
        websocket_ssl_stream_->handshake(host_, "/ws");
        
        // Set connected flag and start reading
        connected_ = true;
        
        // Start the IO thread
        keep_running_ = true;
        io_thread_ = std::thread(&BinanceConnector::runIoService, this);
        
        std::cout << "[Binance] Connected successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        reportError(std::string("Connection error: ") + e.what());
        return false;
    }
}

void BinanceConnector::disconnect() {
    if (!connected_) {
        return;
    }
    
    try {
        std::cout << "[Binance] Disconnecting..." << std::endl;
        
        // Set the stopping flag
        keep_running_ = false;
        
        // Close the WebSocket if it's open
        if (websocket_ssl_stream_) {
            websocket_ssl_stream_->close(websocket::close_code::normal);
        }
        
        // Stop the IO context and join the thread
        io_context_.stop();
        
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        
        websocket_ssl_stream_.reset();
        connected_ = false;
        
        std::cout << "[Binance] Disconnected" << std::endl;
    }
    catch (const std::exception& e) {
        reportError(std::string("Disconnection error: ") + e.what());
    }
}

bool BinanceConnector::subscribe(const SubscriptionRequest& request) {
    if (!connected_) {
        reportError("Cannot subscribe, not connected");
        return false;
    }
    
    try {
        // Store the subscription for potential reconnections
        addSubscriptionRequest(request);
        
        // Create subscription messages for each trading pair and data type
        for (const auto& pair : request.trading_pairs) {
            for (const auto& type : request.data_types) {
                std::string stream;
                
                // Format the stream based on data type
                if (type == DataType::ORDER_BOOK) {
                    // Determine depth update stream based on requested depth
                    if (request.depth <= 10) {
                        stream = pair + "@depth10@100ms";
                    } else if (request.depth <= 20) {
                        stream = pair + "@depth20@100ms";
                    } else {
                        stream = pair + "@depth@100ms";
                    }
                    
                    // Also subscribe to book ticker for best bid/ask updates
                    subscribeToStream(pair + "@bookTicker");
                }
                else if (type == DataType::TRADES) {
                    stream = pair + "@trade";
                }
                else if (type == DataType::TICKER) {
                    stream = pair + "@ticker";
                }
                
                // Subscribe to the stream
                if (!stream.empty()) {
                    subscribeToStream(stream);
                }
            }
        }
        
        return true;
    }
    catch (const std::exception& e) {
        reportError(std::string("Subscription error: ") + e.what());
        return false;
    }
}

void BinanceConnector::subscribeToStream(const std::string& stream) {
    // Compose subscription message
    json subscription = {
        {"method", "SUBSCRIBE"},
        {"params", {stream}},
        {"id", subscription_id_counter_++}
    };
    
    std::string sub_msg = subscription.dump();
    std::cout << "[Binance] Subscribing to: " << stream << std::endl;
    
    // Send the subscription message
    websocket_ssl_stream_->write(boost::asio::buffer(sub_msg));
    
    // Store the stream for reconnection
    subscribed_streams_.push_back(stream);
}

void BinanceConnector::runIoService() {
    try {
        // Set up the read buffer
        beast::flat_buffer buffer;
        
        while (keep_running_ && connected_) {
            try {
                // Read a message into the buffer
                websocket_ssl_stream_->read(buffer);
                
                // Extract the message
                std::string msg = boost::beast::buffers_to_string(buffer.data());
                buffer.consume(buffer.size());
                
                // Process the message
                onMessage(msg);
            }
            catch (const boost::system::system_error& e) {
                if (keep_running_) {
                    reportError(std::string("WebSocket error: ") + e.what(), true);
                    break;
                }
            }
        }
    }
    catch (const std::exception& e) {
        reportError(std::string("IO thread error: ") + e.what(), true);
    }
    
    connected_ = false;
}

void BinanceConnector::onMessage(const std::string& msg) {
    try {
        auto received_time = std::chrono::system_clock::now().time_since_epoch().count();
        json data = json::parse(msg);
        
        // Check if this is a subscription response
        if (data.contains("id") && data.contains("result")) {
            // Just log the subscription result
            std::cout << "[Binance] Subscription response: " << msg << std::endl;
            return;
        }
        
        // Process market data update
        if (data.contains("stream")) {
            // Extract the stream name and data
            std::string stream = data["stream"];
            json stream_data = data["data"];
            
            // Determine the type of data and extract trading pair
            std::string type;
            std::string trading_pair;
            
            size_t at_pos = stream.find('@');
            if (at_pos != std::string::npos) {
                trading_pair = stream.substr(0, at_pos);
                type = stream.substr(at_pos + 1);
            }
            
            // Convert trading pair to uppercase
            std::transform(trading_pair.begin(), trading_pair.end(), trading_pair.begin(), 
                           [](unsigned char c){ return std::toupper(c); });
            
            if (type.find("depth") != std::string::npos) {
                // Handle order book update
                handleOrderBookUpdate(stream_data, trading_pair);
            }
            else if (type == "trade") {
                // Handle trade update
                handleTradeUpdate(stream_data, trading_pair);
            }
            else if (type == "ticker") {
                // Handle ticker update
                handleTickerUpdate(stream_data, trading_pair);
            }
            else if (type == "bookTicker") {
                // Handle book ticker update
                handleBookTickerUpdate(stream_data, trading_pair);
            }
        }
        else {
            // Direct data feed (not wrapped in a stream object)
            if (data.contains("e")) {
                std::string event_type = data["e"];
                std::string trading_pair;
                
                if (data.contains("s")) {
                    trading_pair = data["s"].get<std::string>();
                }
                
                if (event_type == "depthUpdate") {
                    handleOrderBookUpdate(data, trading_pair);
                }
                else if (event_type == "trade") {
                    handleTradeUpdate(data, trading_pair);
                }
                else if (event_type == "24hrTicker") {
                    handleTickerUpdate(data, trading_pair);
                }
            }
        }
    }
    catch (const std::exception& e) {
        reportError(std::string("Message processing error: ") + e.what());
        std::cerr << "Problematic message: " << msg << std::endl;
    }
}

void BinanceConnector::handleOrderBookUpdate(const json& data, const std::string& symbol) {
    try {
        MarketDataUpdate update;
        update.exchange_id = "binance";
        update.trading_pair = symbol;
        update.update_type = "orderbook";
        
        // Extract timestamps
        if (data.contains("E")) {
            update.exchange_timestamp = data["E"];
        } else {
            update.exchange_timestamp = 0;
        }
        
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Serialize the entire JSON update for the order book reconstructor
        std::string json_str = data.dump();
        update.data = std::vector<uint8_t>(json_str.begin(), json_str.end());
        
        // Call the callback function with the update
        data_callback_(update);
    }
    catch (const std::exception& e) {
        reportError(std::string("Order book update error: ") + e.what());
    }
}

void BinanceConnector::handleTradeUpdate(const json& data, const std::string& symbol) {
    try {
        MarketDataUpdate update;
        update.exchange_id = "binance";
        update.trading_pair = symbol;
        update.update_type = "trade";
        
        // Extract timestamps
        if (data.contains("E")) {
            update.exchange_timestamp = data["E"];
        } else if (data.contains("T")) {
            update.exchange_timestamp = data["T"];
        } else {
            update.exchange_timestamp = 0;
        }
        
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Serialize the entire JSON update
        std::string json_str = data.dump();
        update.data = std::vector<uint8_t>(json_str.begin(), json_str.end());
        
        // Call the callback function with the update
        data_callback_(update);
    }
    catch (const std::exception& e) {
        reportError(std::string("Trade update error: ") + e.what());
    }
}

void BinanceConnector::handleTickerUpdate(const json& data, const std::string& symbol) {
    try {
        MarketDataUpdate update;
        update.exchange_id = "binance";
        update.trading_pair = symbol;
        update.update_type = "ticker";
        
        // Extract timestamps
        if (data.contains("E")) {
            update.exchange_timestamp = data["E"];
        } else {
            update.exchange_timestamp = 0;
        }
        
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Serialize the entire JSON update
        std::string json_str = data.dump();
        update.data = std::vector<uint8_t>(json_str.begin(), json_str.end());
        
        // Call the callback function with the update
        data_callback_(update);
    }
    catch (const std::exception& e) {
        reportError(std::string("Ticker update error: ") + e.what());
    }
}

void BinanceConnector::handleBookTickerUpdate(const json& data, const std::string& symbol) {
    try {
        MarketDataUpdate update;
        update.exchange_id = "binance";
        update.trading_pair = symbol;
        update.update_type = "bookTicker";
        
        // BookTicker doesn't typically have a timestamp
        update.exchange_timestamp = 0;
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Serialize the entire JSON update
        std::string json_str = data.dump();
        update.data = std::vector<uint8_t>(json_str.begin(), json_str.end());
        
        // Call the callback function with the update
        data_callback_(update);
    }
    catch (const std::exception& e) {
        reportError(std::string("Book ticker update error: ") + e.what());
    }
}

} // namespace data_ingestion
} // namespace trading