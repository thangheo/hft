#ifndef BINANCE_INTERFACE_H
#define BINANCE_INTERFACE_H

#include "exchange_interface.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
// #include "../../common/order_data.h"
#include <yaml-cpp/yaml.h>

class BinanceInterface : public IExchangeInterface {
public:
    BinanceInterface(const YAML::Node& config);
    ~BinanceInterface();

    bool initialize() override;
    bool sendOrder(Order& order) override;
    void setOrderUpdateCallback(const std::function<void(const Order&)>& callback) override;
    void shutdown() override;

private:
 // Members initialized from config
    std::string exchange_name_;
    std::string api_key_;
    std::string api_secret_;
    std::string base_url_;
    std::string ws_url_;
    std::vector<std::string> symbols_;
    std::vector<std::string> order_types_;
    int requests_per_minute_;
    int max_order_qty_;
    void orderUpdateThread();
    void onOrderUpdate(const Order& order);  
    std::function<void(const Order&)> order_update_callback_;
    std::mutex callback_mutex_;
    std::atomic<bool> running_;
    std::thread update_thread_;

    // Add members for Binance API client
    // e.g., BinanceAPIClient api_client_;
};

#endif // BINANCE_INTERFACE_H
