// #include "mean_reversion_strategy.h"
// #include <iostream>
// #include "../../oms/oms.h" // Assuming OMS is accessible
// #include "../../common/order_data.h" // Assuming Order is accessible
// #include "../../common/market_data.h" // Assuming MarketData is accessible
// MeanReversionStrategy::MeanReversionStrategy()
//     : moving_average_(0.0), window_size_(20) {}

// MeanReversionStrategy::~MeanReversionStrategy() {}

// bool MeanReversionStrategy::initialize(const std::string& config_file) {
//     // Load configuration (e.g., window size, symbol)
//     symbol_ = "btcusdt";
//     window_size_ = 20; // Example window size
//     return true;
// }

// void MeanReversionStrategy::onMarketData(const MarketData& data) {
//     if (data.symbol != symbol_) {
//         return;
//     }

//     // Use the mid-price as the current price
//     if (!data.bids.empty() && !data.asks.empty()) {
//         double bid_price = data.bids.front().price;
//         double ask_price = data.asks.front().price;
//         double mid_price = (bid_price + ask_price) / 2.0;

//         calculateMovingAverage(mid_price);

//         // Strategy logic
//          if (mid_price < moving_average_) {
//             // Prepare order
//             Order order;
//             order.symbol = symbol_;
//             order.exchange = "binance"; // Specify the exchange
//             order.side = "BUY";
//             order.price = ask_price;
//             order.quantity = 0.1;

//             // Send order via OMS
//             OMS::getInstance().sendOrder(order);
//         }
//         else if (mid_price > moving_average_) {
//             // Price is above average, consider selling
//             // Place a sell order
//             Order order;
//             order.symbol = symbol_;
//             order.side = "SELL";
//             order.price = bid_price;
//             order.quantity = 0.1;
//             // Send order to OMS
//             // OMS::getInstance().sendOrder(order);
//         }
//     }
// }

// void MeanReversionStrategy::onOrderUpdate(const Order& order) {
//     // Handle order execution reports
// }

// void MeanReversionStrategy::shutdown() {
//     // Cleanup resources
// }

// void MeanReversionStrategy::calculateMovingAverage(double price) {
//     price_window_.push(price);
//     if (price_window_.size() > window_size_) {
//         price_window_.pop();
//     }

//     // Recalculate moving average
//     double sum = 0.0;
//     for (const auto& p : price_window_) {
//         sum += p;
//     }
//     moving_average_ = sum / price_window_.size();
// }

#include "mean_reversion_strategy.h"
#include <iostream>
#include "../../oms/oms.h" // Assuming OMS is accessible
#include "../../common/order_data.h" // Assuming Order is accessible
#include "../../common/market_data.h" // Assuming MarketData is accessible
#include "spdlog/spdlog.h" // For logging, assuming spdlog is integrated
#include <fstream>
#include <nlohmann/json.hpp> // Assuming nlohmann/json is used for config parsing

std::string statusToString(const OrderStatus& status) {
    // Map your OrderStatus enum or struct to string
    switch(status) {
        case OrderStatus::FILLED:
            return "Filled";
        case OrderStatus::CANCELED:
            return "Canceled";
        case OrderStatus::REJECTED:
            return "Rejected";
        default:
            return "Unknown";
    }
}

// MeanReversionStrategy::MeanReversionStrategy()
//     : moving_average_(0.0), sum_(0.0), window_size_(20) {}

MeanReversionStrategy::MeanReversionStrategy(const YAML::Node& config) {
    strategy_name_ = config["strategy_name"].as<std::string>();
    symbols_ = config["symbols"].as<std::vector<std::string>>();
    window_size_ = config["parameters"]["window_size"].as<int>();
    entry_threshold_ = config["parameters"]["entry_threshold"].as<double>();
    exit_threshold_ = config["parameters"]["exit_threshold"].as<double>();
    order_quantity_ = config["parameters"]["order_quantity"].as<double>();
    // Continue initialization...
}

MeanReversionStrategy::~MeanReversionStrategy() {}

bool MeanReversionStrategy::initialize(const std::string& config_file) {
    // Load configuration from JSON file
    try {
        std::ifstream config_stream(config_file);
        if (!config_stream.is_open()) {
            spdlog::error("Failed to open config file: {}", config_file);
            return false;
        }

        nlohmann::json config_json;
        config_stream >> config_json;

        // Load parameters from the configuration file
        symbol_ = config_json.value("symbol", "btcusdt");
        window_size_ = config_json.value("window_size", 20);
        spdlog::info("Initialized strategy for symbol: {} with window size: {}", symbol_, window_size_);
    } catch (const std::exception& e) {
        spdlog::error("Error loading configuration: {}", e.what());
        return false;
    }
    
    return true;
}

void MeanReversionStrategy::onMarketData(const MarketData& data) {
    if (data.symbol != symbol_) {
        spdlog::debug("Received market data for different symbol: {}", data.symbol);
        return;
    }

    if (data.bids.empty() || data.asks.empty()) {
        spdlog::warn("Market data for {} contains empty bids or asks", symbol_);
        return;
    }

    // Use the mid-price as the current price
    double bid_price = data.bids.front().price;
    double ask_price = data.asks.front().price;
    double mid_price = (bid_price + ask_price) / 2.0;

    calculateMovingAverage(mid_price);

    // Strategy logic: Mean Reversion
    if (mid_price < moving_average_) {
        // Price is below average, buy signal
        Order order;
        order.symbol = symbol_;
        order.exchange = "binance";
        order.side = "BUY";
        order.price = ask_price;
        order.quantity = 0.1;

        spdlog::info("Submitting BUY order: Price = {}, Quantity = {}", order.price, order.quantity);
        OMS::getInstance().sendOrder(order);
    } else if (mid_price > moving_average_) {
        // Price is above average, sell signal
        Order order;
        order.symbol = symbol_;
        order.exchange = "binance";
        order.side = "SELL";
        order.price = bid_price;
        order.quantity = 0.1;

        spdlog::info("Submitting SELL order: Price = {}, Quantity = {}", order.price, order.quantity);
        OMS::getInstance().sendOrder(order);
    }
}

void MeanReversionStrategy::onOrderUpdate(const Order& order) {
    // Handle order updates (filled, canceled, rejected, etc.)
    // spdlog::info("Order update received: {} - Status: {}", order.symbol, order.status);
    spdlog::info("Order update received: {} - Status: {}", order.symbol, statusToString(order.status));
    // Additional logic for handling order updates
}

void MeanReversionStrategy::shutdown() {
    // Cleanup any resources if necessary
    spdlog::info("Shutting down Mean Reversion Strategy for symbol: {}", symbol_);
}

void MeanReversionStrategy::calculateMovingAverage(double price) {
    price_window_.push(price);
    sum_ += price;

    if (price_window_.size() > window_size_) {
        sum_ -= price_window_.front();
        price_window_.pop();
    }

    moving_average_ = sum_ / price_window_.size();
    spdlog::debug("Updated moving average: {}", moving_average_);
}
