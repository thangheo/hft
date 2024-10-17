// adapters/binance_adapter.cpp

#include "binance_adapter.h"
#include <iostream>

BinanceAdapter::BinanceAdapter() {
    // Initialize WebSocket client with Binance endpoint
    ws_client_.setEndpoint("wss://stream.binance.com:9443/ws");
}

BinanceAdapter::~BinanceAdapter() {
    disconnect();
}

bool BinanceAdapter::connect() {
    ws_client_.setMessageCallback([this](const std::string& message) {
        if (data_callback_) {
            data_callback_(message);
        }
    });
    return ws_client_.connect();
}

void BinanceAdapter::disconnect() {
    ws_client_.disconnect();
}

void BinanceAdapter::subscribe(const std::string& symbol) {
    // Binance requires lowercase symbols and appending "@depth"
    std::string stream = symbol + "@depth";
    ws_client_.subscribe(stream);
}

void BinanceAdapter::unsubscribe(const std::string& symbol) {
    std::string stream = symbol + "@depth";
    ws_client_.unsubscribe(stream);
}

void BinanceAdapter::setDataCallback(const std::function<void(const std::string&)>& callback) {
    data_callback_ = callback;
}
