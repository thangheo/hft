#include "binance_interface.h"
#include <iostream>
#include <chrono>
//spdlog
#include <spdlog/spdlog.h>
BinanceInterface::BinanceInterface(const YAML::Node& config) {
    exchange_name_ = config["exchange_name"].as<std::string>();

    // Retrieve API keys from environment variables
    api_key_ = std::getenv("BINANCE_API_KEY") ? std::getenv("BINANCE_API_KEY") : "";
    api_secret_ = std::getenv("BINANCE_API_SECRET") ? std::getenv("BINANCE_API_SECRET") : "";

    if (api_key_.empty() || api_secret_.empty()) {
        std::cerr << "API Key or Secret not set in environment variables." << std::endl;
        // Handle error appropriately
        spdlog::error("API Key or Secret not set in environment variables.");
    }

    base_url_ = config["base_url"].as<std::string>();
    ws_url_ = config["ws_url"].as<std::string>();
    symbols_ = config["symbols"].as<std::vector<std::string>>();
    order_types_ = config["order_types"].as<std::vector<std::string>>();
    requests_per_minute_ = config["rate_limits"]["requests_per_minute"].as<int>();

    // Continue initialization...
    

}
BinanceInterface::~BinanceInterface() {
    shutdown();
}

bool BinanceInterface::initialize() {
    // Initialize Binance API client
    // api_client_.initialize(api_key, api_secret);

    // Start order update thread
    running_ = true;
    update_thread_ = std::thread(&BinanceInterface::orderUpdateThread, this);
    return true;
}

bool BinanceInterface::sendOrder(Order& order) {
    // Map internal order to Binance order format
    // Send order via Binance API client
    // auto response = api_client_.placeOrder(...);

    // For the sake of example, we'll simulate an immediate fill
    order.order_id = "binance_order_id_123";
    order.status = OrderStatus::FILLED;

    // Notify OMS of the order update

    onOrderUpdate(order);

    return true;
}

void BinanceInterface::setOrderUpdateCallback(const std::function<void(const Order&)>& callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    order_update_callback_ = callback;
}

void BinanceInterface::onOrderUpdate(const Order& order) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (order_update_callback_) {
        order_update_callback_(order);
    }
}

void BinanceInterface::orderUpdateThread() {
    while (running_) {
        // Poll for order updates from Binance
        // auto updates = api_client_.getOrderUpdates();

        // For each update, create an Order object and call onOrderUpdate()
        // ...

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void BinanceInterface::shutdown() {
    running_ = false;
    if (update_thread_.joinable()) {
        update_thread_.join();
    }
    // Clean up Binance API client
}
