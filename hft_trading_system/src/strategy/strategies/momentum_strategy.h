#ifndef MOMENTUM_STRATEGY_H
#define MOMENTUM_STRATEGY_H

#include "../strategy_interface.h"
#include <vector>

class MomentumStrategy : public IStrategy {
public:
    MomentumStrategy();
    ~MomentumStrategy();

    bool initialize(const std::string& config_file) override;
    void onMarketData(const MarketData& data) override;
    void onOrderUpdate(const Order& order) override;
    void shutdown() override;

private:
    void calculateMomentum(double price);

    std::vector<double> price_history_;
    size_t window_size_;
    std::string symbol_;
    // Additional parameters
};

#endif // MOMENTUM_STRATEGY_H
