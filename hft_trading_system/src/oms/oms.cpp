#include "oms.h"
#include <iostream>
#include "exchange_interfaces/binance_interface.h"
#include "exchange_interfaces/coinbase_interface.h"

OMS::OMS() {}

OMS::~OMS() {
    shutdown();
}

bool OMS::initialize() {
    // Initialize exchange interfaces
    auto binance_interface = std::make_shared<BinanceInterface>();
    if (!binance_interface->initialize()) {
        std::cerr << "Failed to initialize Binance interface" << std::endl;
        return false;
    }
    exchange_interfaces_["binance"] = binance_interface;

    auto coinbase_interface = std::make_shared<CoinbaseInterface>();
    if (!coinbase_interface->initialize()) {
        std::cerr << "Failed to initialize Coinbase interface" << std::endl;
        return false;
    }
    exchange_interfaces_["coinbase"] = coinbase_interface;

    // Initialize Risk Manager
    risk_manager_ = std::make_shared<RiskManager>();
    if (!risk_manager_->initialize()) {
        std::cerr << "Failed to initialize Risk Manager" << std::endl;
        return false;
    }

    // Set order update callbacks
    for (auto& pair : exchange_interfaces_) {
        pair.second->setOrderUpdateCallback([this](const Order& order) {
            onOrderUpdate(order);
        });
    }

    return true;
}

bool OMS::sendOrder(Order& order) {
    // Perform risk checks
    if (!risk_manager_->preTradeCheck(order)) {
        std::cerr << "Order failed risk checks" << std::endl;
        order.status = OrderStatus::REJECTED;
        onOrderUpdate(order);
        return false;
    }

    // Find the appropriate exchange interface
    auto it = exchange_interfaces_.find(order.exchange);
    if (it == exchange_interfaces_.end()) {
        std::cerr << "Exchange interface not found: " << order.exchange << std::endl;
        order.status = OrderStatus::REJECTED;
        onOrderUpdate(order);
        return false;
    }

    // Send the order
    return it->second->sendOrder(order);
}

void OMS::setOrderUpdateCallback(const std::function<void(const Order&)>& callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    order_update_callback_ = callback;
}

void OMS::onOrderUpdate(const Order& order) {
    // Pass the order update to the Strategy Engine or other subscribers
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (order_update_callback_) {
        order_update_callback_(order);
    }
}

void OMS::shutdown() {
    for (auto& pair : exchange_interfaces_) {
        pair.second->shutdown();
    }
    exchange_interfaces_.clear();

    if (risk_manager_) {
        risk_manager_->shutdown();
        risk_manager_.reset();
    }
}
