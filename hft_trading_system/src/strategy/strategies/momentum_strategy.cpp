#include "momentum_strategy.h"
#include <iostream>
#include "../../oms/oms.h"

MomentumStrategy::MomentumStrategy()
    : window_size_(20) {}

MomentumStrategy::~MomentumStrategy() {}

bool MomentumStrategy::initialize(const std::string& config_file) {
    // Load configuration
    symbol_ = "btcusdt";
    window_size_ = 20;
    return true;
}

void MomentumStrategy::onMarketData(const MarketData& data) {
    if (data.symbol != symbol_) {
        return;
    }

    if (!data.bids.empty() && !data.asks.empty()) {
        double bid_price = data.bids.front().price;
        double ask_price = data.asks.front().price;
        double mid_price = (bid_price + ask_price) / 2.0;

        calculateMomentum(mid_price);

        // Strategy logic
        if (price_history_.size() >= window_size_) {
            double momentum = mid_price - price_history_.front();
            if (momentum > 0) {
                // Upward momentum, consider buying
                Order order;
                order.symbol = symbol_;
                order.side = "BUY";
                order.price = ask_price;
                order.quantity = 0.1;
                // OMS::getInstance().sendOrder(order);
            } else if (momentum < 0) {
                // Downward momentum, consider selling
                Order order;
                order.symbol = symbol_;
                order.side = "SELL";
                order.price = bid_price;
                order.quantity = 0.1;
                // OMS::getInstance().sendOrder(order);
            }
        }
    }
}

void MomentumStrategy::onOrderUpdate(const Order& order) {
    // Handle order execution reports
}

void MomentumStrategy::shutdown() {
    // Cleanup resources
}

void MomentumStrategy::calculateMomentum(double price) {
    price_history_.push_back(price);
    if (price_history_.size() > window_size_) {
        price_history_.erase(price_history_.begin());
    }
}
