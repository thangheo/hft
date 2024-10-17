// main.cpp

#include "market_data/market_data_handler.h"
#include "strategy/strategy_engine.h"
#include <chrono>
#include <thread>
int main() {
    MarketDataHandler market_data_handler;

    // Set up callback to receive market data
    market_data_handler.setMarketDataCallback([](const MarketData& data) {
        // Process market data
        // For example, pass it to the Strategy Engine
        // strategyEngine.onMarketData(data);
    });

    // Connect to exchanges
    market_data_handler.connect("binance");
    // market_data_handler.connect("coinbase");

    // Subscribe to symbols
    market_data_handler.subscribe("btcusdt");
    // market_data_handler.subscribe("ethusdt");

    // Keep the application running
    while (true) {
        // Main loop or event processing
        // Sleep or wait on a condition variable
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
