// storage_manager.h

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <memory>
#include "real_time_data/real_time_db.h"
#include "historical_data/historical_db.h"

class StorageManager {
public:
    StorageManager();
    ~StorageManager();

    bool initialize();
    void shutdown();

    bool storeRealTimeMarketData(const MarketData& data);
    bool storeHistoricalMarketData(const MarketData& data);

    bool storeOrder(const Order& order);

private:
    std::unique_ptr<RealTimeDB> real_time_db_;
    std::unique_ptr<HistoricalDB> historical_db_;
};

#endif // STORAGE_MANAGER_H
