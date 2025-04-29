// src/data_ingestion/main.cpp
#include "exchange_connector.hpp"
#include "binance_connector.hpp"
#include "order_book.hpp"
#include "normalizer.hpp"
#include "publisher.hpp"

#include <memory>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <iostream>
#include <signal.h>

using namespace trading::data_ingestion;

std::atomic<bool> running(true);

void signalHandler(int signal) {
    running = false;
}

int main(int argc, char* argv[]) {
    // Setup signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize publisher
    auto publisher = std::make_shared<Publisher>("tcp://*:5555");
    publisher->start();
    
    // Initialize normalizer
    auto normalizer = std::make_shared<Normalizer>(
        [publisher](const OrderBookUpdate& update) {
            publisher->publishOrderBookUpdate(update);
        }
    );
    
    // Setup order books
    std::map<std::string, std::map<std::string, std::shared_ptr<OrderBook>>> order_books;
    
    // Initialize exchange connectors
    std::vector<std::shared_ptr<ExchangeConnector>> connectors;
    
    // Create Binance connector
    auto binance = std::make_shared<BinanceConnector>(
        "stream.binance.com", "9443",
        [&order_books](const MarketDataUpdate& update) {
            // Find or create order book
            auto& exchange_map = order_books[update.exchange_id];
            if (exchange_map.find(update.trading_pair) == exchange_map.end()) {
                exchange_map[update.trading_pair] = std::make_shared<OrderBook>(
                    update.exchange_id, update.trading_pair,
                    [normalizer](const OrderBookUpdate& update) {
                        normalizer->processOrderBookUpdate(update);
                    }
                );
            }
            
            // Process the update
            exchange_map[update.trading_pair]->processUpdate(update);
        }
    );
    connectors.push_back(binance);
    
    // Connect and subscribe
    for (auto& connector : connectors) {
        if (connector->connect()) {
            SubscriptionRequest request;
            request.exchange_id = connector->getExchangeId();
            request.trading_pairs = {"BTC/USDT", "ETH/USDT", "ADA/USDT", "SOL/USDT"};
            request.data_types = {DataType::ORDER_BOOK};
            request.depth = 20;
            
            connector->subscribe(request);
        }
    }
    
    // Main loop
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Cleanup
    publisher->stop();
    for (auto& connector : connectors) {
        connector->disconnect();
    }
    
    return 0;
}