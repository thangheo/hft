// real_time_db.h

#ifndef REAL_TIME_DB_H
#define REAL_TIME_DB_H

#include <string>
#include <memory>
#include <sw/redis++/redis++.h>
#include "../../common/market_data.h"

class RealTimeDB {
public:
    RealTimeDB();
    ~RealTimeDB();

    bool connect(const std::string& host = "127.0.0.1", int port = 6379);
    void disconnect();

    // Existing methods
    bool storeMarketData(const std::string& key, const std::string& data);
    bool getMarketData(const std::string& key, std::string& data);

    // New methods for caching
    bool cacheMarketData(const MarketData& data);
    bool getCachedMarketData(const std::string& symbol, MarketData& data);

private:
    std::shared_ptr<sw::redis::Redis> redis_;
};

#endif // REAL_TIME_DB_H
