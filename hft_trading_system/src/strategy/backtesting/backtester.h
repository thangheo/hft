#ifndef BACKTESTER_H
#define BACKTESTER_H

#include <vector>
#include <string>
#include <memory>
#include "../strategy_interface.h"
#include "../../data_storage/historical_data/historical_db.h"

class Backtester {
public:
    Backtester();
    ~Backtester();

    bool initialize();
    void runBacktest(const std::string& symbol, 
                     const std::chrono::system_clock::time_point& start_time, 
                     const std::chrono::system_clock::time_point& end_time);
    void addStrategy(std::shared_ptr<IStrategy> strategy);
    void removeStrategy(std::shared_ptr<IStrategy> strategy);

private:
    std::vector<std::shared_ptr<IStrategy>> strategies_;
    HistoricalDB historical_db_;
};

#endif // BACKTESTER_H
