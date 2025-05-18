// src/data_ingestion/websocket/exchanges/binance_websocket_client.cpp
#include "data_ingestion/websocket/exchanges/binance_websocket_client.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace trading {
namespace data_ingestion {

BinanceWebSocketClient::BinanceWebSocketClient(const std::string& host,
                                             const std::string& port,
                                             MarketDataCallback callback)
    : WebSocketClientBase(host, port, true),
      data_callback_(std::move(callback)) {
    
    // Configure connection parameters
    setPingInterval(30000);  // 30 seconds ping interval
    setReconnectionParameters(5, 1000, 1.5);  // 5 attempts, starting with 1s delay
}

BinanceWebSocketClient::~BinanceWebSocketClient() {
    disconnect();
}

void WebSocketClientBase::onOpen() {
    // Default empty implementation

}

void WebSocketClientBase::onMessage(const std::string& message) {
    // Default empty implementation
}

void WebSocketClientBase::onClose(int code, const std::string& reason) {
    // Default empty implementation
}
void WebSocketClientBase::onError(const std::string& error) {
    // Default empty implementation
}
void WebSocketClientBase::onReconnect() {
    // Default empty implementation
}

bool BinanceWebSocketClient::isConnected() const {
    return WebSocketClientBase::isConnected();
}

bool BinanceWebSocketClient::subscribe(const std::vector<std::string>& streams) {
    if (!isConnected()) {
        std::cerr << "Cannot subscribe, not connected to Binance" << std::endl;
        return false;
    }
    
    return sendSubscriptionRequest(streams, true);
}

bool BinanceWebSocketClient::unsubscribe(const std::vector<std::string>& streams) {
    if (!isConnected()) {
        std::cerr << "Cannot unsubscribe, not connected to Binance" << std::endl;
        return false;
    }
    
    return sendSubscriptionRequest(streams, false);
}

std::vector<std::string> BinanceWebSocketClient::getSubscribedStreams() const {
    // Return a copy of the subscribed streams
    // to avoid
    // concurrent modification issues
    // std::lock_guard<std::mutex> lock(subscription_mutex_);
    return subscribed_streams_; 
}
void BinanceWebSocketClient::onOpen() {
    std::cout << "Connected to Binance WebSocket" << std::endl;
    
    // Resubscribe to previously subscribed streams
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    if (!subscribed_streams_.empty()) {
        std::cout << "Resubscribing to " << subscribed_streams_.size() << " streams" << std::endl;
        subscribe(subscribed_streams_);
    }
}

void BinanceWebSocketClient::onMessage(const std::string& message) {
    try {
        // Parse the JSON message
        json data = json::parse(message);
        
        // Handle subscription responses
        if (data.contains("id") && (data.contains("result") || data.contains("error"))) {
            handleResponse(data);
            return;
        }
        
        // Handle stream messages
        if (data.contains("stream") && data.contains("data")) {
            handleStreamMessage(data);
            return;
        }
        
        // Handle direct data format (not wrapped in stream)
        if (data.contains("e")) {
            std::string event_type = data["e"];
            std::string symbol = data.contains("s") ? data["s"] : "";
            
            if (event_type == "depthUpdate") {
                processOrderBookData(symbol, "depth", data);
            } else if (event_type == "trade") {
                processTrade(symbol, data);
            } else if (event_type == "24hrTicker") {
                processTicker(symbol, data);
            } else if (event_type == "bookTicker") {
                processBookTicker(symbol, data);
            }
            return;
        }
        
        std::cout << "Unhandled Binance message: " << message << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing Binance message: " << e.what() << std::endl;
        std::cerr << "Message: " << message << std::endl;
    }
}

void BinanceWebSocketClient::onClose(int code, const std::string& reason) {
    std::cout << "Disconnected from Binance WebSocket: [" << code << "] " << reason << std::endl;
}

void BinanceWebSocketClient::onError(const std::string& error) {
    std::cerr << "Binance WebSocket error: " << error << std::endl;
}

void BinanceWebSocketClient::onReconnect() {
    std::cout << "Reconnected to Binance WebSocket" << std::endl;
    
    // Resubscriptions are handled in onOpen()
}

void BinanceWebSocketClient::handleStreamMessage(const json& data) {
    try {
        std::string stream = data["stream"];
        json stream_data = data["data"];
        
        // Extract symbol and event type from the stream name
        std::string symbol = extractSymbolFromStream(stream);
        std::string event_type = extractEventTypeFromStream(stream);
        
        // Process based on the event type
        if (event_type.find("depth") != std::string::npos) {
            processOrderBookData(symbol, event_type, stream_data);
        }
        else if (event_type == "trade") {
            processTrade(symbol, stream_data);
        }
        else if (event_type == "ticker") {
            processTicker(symbol, stream_data);
        }
        else if (event_type == "bookTicker") {
            processBookTicker(symbol, stream_data);
        }
        else {
            std::cout << "Unhandled Binance stream type: " << event_type << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing Binance stream message: " << e.what() << std::endl;
    }
}

void BinanceWebSocketClient::handleResponse(const json& data) {
    try {
        int id = data["id"];
        
        if (data.contains("result")) {
            // Successful response
            std::cout << "Binance subscription response [" << id << "]: Success" << std::endl;
        }
        else if (data.contains("error")) {
            // Error response
            std::cerr << "Binance subscription error [" << id << "]: " 
                      << data["error"]["code"] << " - " << data["error"]["msg"] << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing Binance response: " << e.what() << std::endl;
    }
}

void BinanceWebSocketClient::processOrderBookData(const std::string& symbol, 
                                                const std::string& event_type, 
                                                const json& data) {
    if (!data_callback_) {
        return;
    }
    
    try {
        MarketDataUpdate update;
        update.exchangeId = "binance";
        update.trading_pair = symbol;
        update.update_type = "orderbook";
        
        // Extract timestamps
        if (data.contains("E")) {
            update.exchange_timestamp = data["E"];
        } else {
            update.exchange_timestamp = 0;
        }
        
        // Set received timestamp
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Serialize the data for processing
        std::string json_str = data.dump();
        update.data = std::vector<uint8_t>(json_str.begin(), json_str.end());
        
        // Call the callback
        data_callback_(update);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing Binance order book data: " << e.what() << std::endl;
    }
}

void BinanceWebSocketClient::processTrade(const std::string& symbol, const json& data) {
    if (!data_callback_) {
        return;
    }
    
    try {
        MarketDataUpdate update;
        update.exchangeId = "binance";
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
        
        // Set received timestamp
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Serialize the data for processing
        std::string json_str = data.dump();
        update.data = std::vector<uint8_t>(json_str.begin(), json_str.end());
        
        // Call the callback
        data_callback_(update);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing Binance trade data: " << e.what() << std::endl;
    }
}

void BinanceWebSocketClient::processTicker(const std::string& symbol, const json& data) {
    if (!data_callback_) {
        return;
    }
    
    try {
        MarketDataUpdate update;
        update.exchangeId = "binance";
        update.trading_pair = symbol;
        update.update_type = "ticker";
        
        // Extract timestamps
        if (data.contains("E")) {
            update.exchange_timestamp = data["E"];
        } else {
            update.exchange_timestamp = 0;
        }
        
        // Set received timestamp
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Serialize the data for processing
        std::string json_str = data.dump();
        update.data = std::vector<uint8_t>(json_str.begin(), json_str.end());
        
        // Call the callback
        data_callback_(update);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing Binance ticker data: " << e.what() << std::endl;
    }
}

void BinanceWebSocketClient::processBookTicker(const std::string& symbol, const json& data) {
    if (!data_callback_) {
        return;
    }
    
    try {
        MarketDataUpdate update;
        update.exchangeId = "binance";
        update.trading_pair = symbol;
        update.update_type = "bookTicker";
        
        // BookTicker doesn't typically include a timestamp
        update.exchange_timestamp = 0;
        
        // Set received timestamp
        update.received_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Serialize the data for processing
        std::string json_str = data.dump();
        update.data = std::vector<uint8_t>(json_str.begin(), json_str.end());
        
        // Call the callback
        data_callback_(update);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing Binance book ticker data: " << e.what() << std::endl;
    }
}

bool BinanceWebSocketClient::sendSubscriptionRequest(const std::vector<std::string>& streams, bool subscribe) {
    try {
        // Prepare the subscription message
        json request;
        request["id"] = subscription_id_counter_++;
        request["method"] = subscribe ? "SUBSCRIBE" : "UNSUBSCRIBE";
        request["params"] = streams;
        
        // Send the request
        std::string message = request.dump();
        send(message);
        
        // Update subscription list
        std::lock_guard<std::mutex> lock(subscription_mutex_);
        if (subscribe) {
            // Add new streams to the list
            for (const auto& stream : streams) {
                if (std::find(subscribed_streams_.begin(), subscribed_streams_.end(), stream) == subscribed_streams_.end()) {
                    subscribed_streams_.push_back(stream);
                }
            }
        } else {
            // Remove streams from the list
            for (const auto& stream : streams) {
                auto it = std::find(subscribed_streams_.begin(), subscribed_streams_.end(), stream);
                if (it != subscribed_streams_.end()) {
                    subscribed_streams_.erase(it);
                }
            }
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error sending Binance subscription request: " << e.what() << std::endl;
        return false;
    }
}

std::string BinanceWebSocketClient::extractSymbolFromStream(const std::string& stream) const {
    size_t at_pos = stream.find('@');
    if (at_pos != std::string::npos) {
        std::string symbol = stream.substr(0, at_pos);
        
        // Convert to uppercase
        std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
        
        return symbol;
    }
    return "";
}

std::string BinanceWebSocketClient::extractEventTypeFromStream(const std::string& stream) const {
    size_t at_pos = stream.find('@');
    if (at_pos != std::string::npos && at_pos + 1 < stream.length()) {
        return stream.substr(at_pos + 1);
    }
    return "";
}


} // namespace data_ingestion
} // namespace trading