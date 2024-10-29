#ifndef OMS_H
#define OMS_H

#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include "../common/order_data.h"
#include "exchange_interfaces/exchange_interface.h"
#include "../risk_management/risk_manager.h"
#include "../../configs/config_loader.h"

class OMS {
public:
    static OMS& getInstance() {
        static OMS instance;
        return instance;
    }

    // Delete copy constructors
    OMS(const OMS&) = delete;
    OMS& operator=(const OMS&) = delete;

    /**
     * @brief Initialize the OMS with a configuration file.
     * @param config Configuration data for setting up exchange interfaces.
     * @return True if initialization is successful.
     */
    bool initialize(const YAML::Node& config);

    /**
     * @brief Submit a new order.
     * @param order The order to submit.
     * @return True if the order is accepted for processing.
     */
    bool sendOrder(Order& order);

    /**
     * @brief Set the callback function for order updates.
     * @param callback The function to call with order updates.
     */
    void setOrderUpdateCallback(const std::function<void(const Order&)>& callback);

    /**
     * @brief Shutdown the OMS.
     */
    void shutdown();

private:
    OMS();
    ~OMS();
    void onOrderUpdate(const Order& order);

    // Map exchange names to their interfaces
    std::unordered_map<std::string, std::shared_ptr<IExchangeInterface>> exchange_interfaces_;
    std::function<void(const Order&)> order_update_callback_;

    std::mutex callback_mutex_;
    std::shared_ptr<RiskManager> risk_manager_;
};

#endif // OMS_H
