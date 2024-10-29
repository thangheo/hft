#include <iostream>  // Add this to include std::cerr
#include "market_data/market_data_handler.h"
#include "strategy/strategy_engine.h"
#include "oms/oms.h"
#include "strategy/strategies/mean_reversion_strategy.h"  // Ensure this is included if defined
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <memory>
#include "../configs/config_loader.h"
#include <yaml-cpp/yaml.h>

//load config from json file

int main() {
    // Initialize ConfigLoader
    ConfigLoader config_loader("configs");

    // Load configurations
    if (!config_loader.loadExchangeConfig("binance")) {
        std::cerr << "Failed to load Binance exchange config" << std::endl;
        return 1;
    }

    if (!config_loader.loadRiskConfig()) {
        std::cerr << "Failed to load risk config" << std::endl;
        return 1;
    }

    if (!config_loader.loadStrategyConfig("mean_reversion")) {
        std::cerr << "Failed to load Mean Reversion strategy config" << std::endl;
        return 1;
    }

    // Access configurations
    YAML::Node binance_config = config_loader.getExchangeConfig("binance");
    YAML::Node risk_config = config_loader.getRiskConfig();
    YAML::Node strategy_config = config_loader.getStrategyConfig("mean_reversion");
     // Pass configurations to modules
    // BinanceInterface binance_interface(binance_config);
    // RiskManager risk_manager(risk_config);
    // MeanReversionStrategy strategy(strategy_config);
    /////////////////////////////


    // Create a registry
    auto registry = std::make_shared<prometheus::Registry>();

    // Initialize components with the registry
    MarketDataHandler market_data_handler(registry);
    // Initialize components
    StrategyEngine strategy_engine;
    OMS& oms = OMS::getInstance();  // Use the singleton instance

    // Initialize strategies
    //load config file for strategy
    // config_loader.loadStrategyConfig("mean_reversion");
    auto mean_reversion_strategy = std::make_shared<MeanReversionStrategy>(strategy_config);
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
