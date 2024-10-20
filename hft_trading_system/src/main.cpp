#include <iostream>  // Add this to include std::cerr
#include "market_data/market_data_handler.h"
#include "strategy/strategy_engine.h"
#include "oms/oms.h"
#include "strategy/strategies/mean_reversion_strategy.h"  // Ensure this is included if defined
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <memory>
int main() {
    // Create a registry
    auto registry = std::make_shared<prometheus::Registry>();

    // Initialize components with the registry
    MarketDataHandler market_data_handler(registry);
    // Initialize components
    StrategyEngine strategy_engine;
    OMS& oms = OMS::getInstance();  // Use the singleton instance

    // Initialize strategies
    auto mean_reversion_strategy = std::make_shared<MeanReversionStrategy>();  // Ensure this is defined
    strategy_engine.registerStrategy(mean_reversion_strategy);

    // Set up callbacks
    market_data_handler.setMarketDataCallback([&strategy_engine](const MarketData& data) {
        strategy_engine.onMarketData(data);
    });

    oms.setOrderUpdateCallback([&strategy_engine](const Order& order) {
        strategy_engine.onOrderUpdate(order);
    });

    // Start components
    if (!market_data_handler.connect("binance")) {
        std::cerr << "Failed to connect to market data source" << std::endl;
        return 1;
    }

    if (!strategy_engine.initialize()) {
        std::cerr << "Failed to initialize strategy engine" << std::endl;
        return 1;
    }

    // Subscribe to symbols
    market_data_handler.subscribe("btcusdt");
    // Expose metrics
    prometheus::Exposer exposer{"0.0.0.0:8081"};
    exposer.RegisterCollectable(registry);
    // Keep the application running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Shutdown components
    strategy_engine.shutdown();
    market_data_handler.disconnect("binance");

    return 0;
}
