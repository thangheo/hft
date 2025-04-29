//just to pass the build process then comeback later

// #include <iostream>
// #include <pqxx/pqxx>
// #include <memory>   
// #include <nlohmann/json.hpp>
// #include <string>
// #include "../common/market_data.h"
// #include "../common/order_data.h"
// #include "../data_storage/real_time_data/real_time_db.h"
// #include "../data_storage/storage_manager.h"

// using json = nlohmann::json;
// // using namespace hft::data_storage;

// //class strategy_engine
// class StrategyEngine {
//     public:
//         StrategyEngine();
//         ~StrategyEngine();

//         void onTick();
//         void onMarketData(const MarketData& data);
//         void onOrderUpdate(const Order& order);
//         // void onExecutionReport(const ExecutionReport& report);

//         void setOrderCallback(const std::function<void(const Order&)>& callback);
//         // void setExecutionReportCallback(const std::function<void(const ExecutionReport&)>& callback);
//         void processMarketData(const MarketData& data);
//     private:
//         std::shared_ptr<RealTimeDB> real_time_db_;
//         std::function<void(const Order&)> order_callback_;
//         // std::function<void(const ExecutionReport&)> execution_report_callback_;

// };
#ifndef STRATEGY_ENGINE_H
#define STRATEGY_ENGINE_H

#include <vector>
#include <memory>
#include <mutex>
#include "../common/market_data.h"
#include "strategy_interface.h"

class StrategyEngine {
public:
    StrategyEngine();
    ~StrategyEngine();
    
        // void onTick();

    /**
     * @brief Register a new strategy with the engine.
     * @param strategy The strategy to register.
     */
    void registerStrategy(std::shared_ptr<IStrategy> strategy);

    /**
     * @brief Unregister a strategy from the engine.
     * @param strategy The strategy to unregister.
     */
    void unregisterStrategy(std::shared_ptr<IStrategy> strategy);

    /**
     * @brief Receive market data from Market Data Handler.
     * @param data The market data.
     */
    void onMarketData(const MarketData& data);

    /**
     * @brief Receive order updates from OMS.
     * @param order The order data.
     */
    void onOrderUpdate(const Order& order);

    /**
     * @brief Initialize the engine.
     */
    bool initialize();

    /**
     * @brief Shutdown the engine.
     */
    void shutdown();

private:
    std::vector<std::shared_ptr<IStrategy>> strategies_;
    std::mutex strategies_mutex_;
};

#endif // STRATEGY_ENGINE_H
