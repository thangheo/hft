// historical_db.h

#ifndef HISTORICAL_DB_H
#define HISTORICAL_DB_H

#include <string>
#include <pqxx/pqxx>
#include <vector>
#include <chrono>
#include "../../common/market_data.h"
#include "../../common/order_data.h"
class HistoricalDB {
public:
    HistoricalDB();
    ~HistoricalDB();

    bool connect(const std::string& connection_string);
    void disconnect();
    //define to_string
   
    bool storeMarketData(const MarketData& data);
    bool storeOrder(const Order& order);
    // Add methods for data retrieval

    std::vector<MarketData> queryMarketData(
        const std::string& symbol,
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time);

    std::vector<Order> queryOrders(
        const std::string& symbol,
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time,
        const std::string& status = "");

private:
    std::unique_ptr<pqxx::connection> conn_;  // Change to unique_ptr
};

#endif // HISTORICAL_DB_H
