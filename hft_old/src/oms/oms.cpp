#include "oms.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include "exchange_interfaces/binance_interface.h"
#include "exchange_interfaces/coinbase_interface.h"

OMS::OMS() {}

OMS::~OMS() {
    shutdown();
}

bool OMS::initialize(const YAML::Node& config) {
    try {
        // Load Binance configuration if present
        if (config["binance"]) {
            auto binance_interface = std::make_shared<BinanceInterface>(config["binance"]);
            if (!binance_interface->initialize()) {
                spdlog::error("Failed to initialize Binance interface");
                return false;
            }
            exchange_interfaces_["binance"] = binance_interface;
        }

        // Load Coinbase configuration if present (assuming it doesn’t need YAML config)
        if (config["coinbase"]) {
            auto coinbase_interface = std::make_shared<CoinbaseInterface>();
            if (!coinbase_interface->initialize()) {
                spdlog::error("Failed to initialize Coinbase interface");
                return false;
            }
            exchange_interfaces_["coinbase"] = coinbase_interface;
        }

        // Initialize Risk Manager
        risk_manager_ = std::make_shared<RiskManager>(config["risk"]);
        if (!risk_manager_->initialize()) {
            spdlog::error("Failed to initialize Risk Manager");
            return false;
        }

        // Set callbacks for each exchange interface
        for (auto& pair : exchange_interfaces_) {
            pair.second->setOrderUpdateCallback([this](const Order& order) {
                onOrderUpdate(order);
            });
        }

        spdlog::info("OMS initialized successfully");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Exception during OMS initialization: {}", e.what());
        return false;
    }
}

bool OMS::sendOrder(Order& order) {
    if (!risk_manager_->preTradeCheck(order)) {
        spdlog::warn("Order failed risk checks: {}", order.order_id);
        order.status = OrderStatus::REJECTED;
        onOrderUpdate(order);
        return false;
    }

    auto it = exchange_interfaces_.find(order.exchange);
    if (it == exchange_interfaces_.end()) {
        spdlog::warn("Exchange interface not found: {}", order.exchange);
        order.status = OrderStatus::REJECTED;
        onOrderUpdate(order);
        return false;
    }

    bool success = it->second->sendOrder(order);
    if (success) {
        spdlog::info("Order sent successfully: {}", order.order_id);
    } else {
        spdlog::error("Failed to send order: {}", order.order_id);
    }

    return success;
}

void OMS::setOrderUpdateCallback(const std::function<void(const Order&)>& callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    order_update_callback_ = callback;
}

void OMS::onOrderUpdate(const Order& order) {
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

    spdlog::info("OMS shutdown completed.");
}
