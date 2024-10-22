#ifndef RISK_MANAGER_H
#define RISK_MANAGER_H

#include <memory>
#include "../common/order_data.h"
#include <yaml-cpp/yaml.h>
class RiskManager {
public:
    RiskManager(const YAML::Node& config);
    ~RiskManager();

    bool initialize();
    bool preTradeCheck(const Order& order);
    void shutdown();

private:
    std::map<std::string, double> max_position_size_;
    std::map<std::string, double> max_order_size_;
    double max_daily_loss_;
    double max_order_value_;
    // Add more risk parameters as needed
    // double max_order_quantity_;
    // double max_order_value_;
    // double max_order_volume_;
    // double max_order_count_;
    // double max_order_frequency_;
    // double max_order_rate_;
    // double max_order_age_;
    // double max_order_lifetime_;
};

#endif // RISK_MANAGER_H
