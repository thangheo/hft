#ifndef COINBASE_INTERFACE_H
#define COINBASE_INTERFACE_H

#include "exchange_interface.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

class CoinbaseInterface : public IExchangeInterface {
public:
    CoinbaseInterface();
    ~CoinbaseInterface();

    bool initialize() override;
    bool sendOrder(Order& order) override;
    void setOrderUpdateCallback(const std::function<void(const Order&)>& callback) override;
    void shutdown() override;

private:
    void orderUpdateThread();
    void onOrderUpdate(const Order& order);  
    std::function<void(const Order&)> order_update_callback_;
    std::mutex callback_mutex_;
    std::atomic<bool> running_;
    std::thread update_thread_;

    // Add members for Coinbase API client
    // e.g., CoinbaseAPIClient api_client_;
};

#endif // COINBASE_INTERFACE_H
