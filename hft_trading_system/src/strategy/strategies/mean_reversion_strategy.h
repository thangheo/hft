// #ifndef MEAN_REVERSION_STRATEGY_H
// #define MEAN_REVERSION_STRATEGY_H

// #include "../strategy_interface.h"
// #include <queue>

// class MeanReversionStrategy : public IStrategy {
// public:
//     MeanReversionStrategy();
//     ~MeanReversionStrategy();

//     bool initialize(const std::string& config_file) override;
//     void onMarketData(const MarketData& data) override;
//     void onOrderUpdate(const Order& order) override;
//     void shutdown() override;

// private:
//     void calculateMovingAverage(double price);

//     double moving_average_;
//     std::queue<double> price_window_;
//     size_t window_size_;
//     std::string symbol_;
//     // Additional parameters
// };

// #endif // MEAN_REVERSION_STRATEGY_H

#ifndef MEAN_REVERSION_STRATEGY_H
#define MEAN_REVERSION_STRATEGY_H

#include "../strategy_interface.h"
#include "../../common/market_data.h"
#include "../../common/order_data.h"
#include <queue>
#include <string>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <spdlog/spdlog.h>

class MeanReversionStrategy : public IStrategy {
public:
    // MeanReversionStrategy();
    MeanReversionStrategy(const YAML::Node& config);
    ~MeanReversionStrategy();

    // Initializes the strategy, loading configurations from the given file
    bool initialize(const std::string& config_file) override;

    // Processes incoming market data and evaluates whether to place trades
    void onMarketData(const MarketData& data) override;

    // Handles updates about orders (e.g., fill, cancelation, rejection)
    void onOrderUpdate(const Order& order) override;

    // Shuts down the strategy, cleaning up resources if necessary
    void shutdown() override;
    //function to reload the configuration file
    void reloadConfig(const std::string& config_file);
    //function to get the strategy name
    std::string getStrategyName();
private:
    std::string strategy_name_;
    std::vector<std::string> symbols_;
    // int window_size_;
    double entry_threshold_;
    double exit_threshold_;
    double order_quantity_;
    // Calculates the moving average based on recent prices
    void calculateMovingAverage(double price);

    // Tracks the current moving average
    double moving_average_;

    // Queue of recent prices for calculating the moving average
    std::queue<double> price_window_;

    // Sum of prices for efficient moving average calculation
    double sum_;

    // Number of prices to include in the moving average
    size_t window_size_;

    // The trading symbol this strategy is operating on (e.g., btcusdt)
    std::string symbol_;
};

#endif // MEAN_REVERSION_STRATEGY_H
