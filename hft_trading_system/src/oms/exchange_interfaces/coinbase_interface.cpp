#include "coinbase_interface.h"
#include <iostream>

CoinbaseInterface::CoinbaseInterface() : running_(false) {}

CoinbaseInterface::~CoinbaseInterface() {
    shutdown();
}

bool CoinbaseInterface::initialize() {
    // Initialize Coinbase API client
    // api_client_.initialize(api_key, api_secret);

    // Start order update thread
    running_ = true;
    update_thread_ = std::thread(&CoinbaseInterface::orderUpdateThread, this);
    return true;
}

bool CoinbaseInterface::sendOrder(Order& order) {
    // Map internal order to Coinbase order format
    // Send order via Coinbase API client
    // auto response = api_client_.placeOrder(...);

    // For the sake of example, we'll simulate an immediate fill
    order.order_id = "coinbase_order_id_123";
    order.status = OrderStatus::FILLED;

    // Notify OMS of the order update
    onOrderUpdate(order);

    return true;
}

void CoinbaseInterface::setOrderUpdateCallback(const std::function<void(const Order&)>& callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    order_update_callback_ = callback;
}

void CoinbaseInterface::onOrderUpdate(const Order& order) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (order_update_callback_) {
        order_update_callback_(order);
    }
}

void CoinbaseInterface::orderUpdateThread() {
    while (running_) {
        // Poll for order updates from Coinbase
        // auto updates = api_client_.getOrderUpdates();

        // For each update, create an Order object and call onOrderUpdate()
        // ...

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void CoinbaseInterface::shutdown() {
    running_ = false;
    if (update_thread_.joinable()) {
        update_thread_.join();
    }
    // Clean up Coinbase API client
}
