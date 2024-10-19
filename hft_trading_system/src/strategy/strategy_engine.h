//just to pass the build process then comeback later

#include <iostream>
#include <pqxx/pqxx>
#include <memory>   
#include <nlohmann/json.hpp>
#include <string>
#include "src/common/market_data.h"
#include "src/common/order_data.h"
#include "src/data_storage/real_time_data/real_time_db.h"
#include "src/data_storage/storage_manager.h"

using json = nlohmann::json;
// using namespace hft::data_storage;

//class strategy_engine
class StrategyEngine {
    public:
        StrategyEngine();
        ~StrategyEngine();

        void onTick();
        void onMarketData(const MarketData& data);
        void onOrderUpdate(const Order& order);
        // void onExecutionReport(const ExecutionReport& report);

        void setOrderCallback(const std::function<void(const Order&)>& callback);
        // void setExecutionReportCallback(const std::function<void(const ExecutionReport&)>& callback);
        void processMarketData(const MarketData& data);
    private:
        std::shared_ptr<RealTimeDB> real_time_db_;
        std::function<void(const Order&)> order_callback_;
        // std::function<void(const ExecutionReport&)> execution_report_callback_;

};
