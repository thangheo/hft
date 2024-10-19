#ifndef RISK_MANAGER_H
#define RISK_MANAGER_H

#include <memory>
#include "../common/order_data.h"

class RiskManager {
public:
    RiskManager();
    ~RiskManager();

    bool initialize();
    bool preTradeCheck(const Order& order);
    void shutdown();

private:
    // Risk parameters and state
};

#endif // RISK_MANAGER_H
