// market_data_handler.h

#ifndef MARKET_DATA_HANDLER_H
#define MARKET_DATA_HANDLER_H

#include "imarket_data_handler.h"
#include "adapters/exchange_adapter.h"
#include "../common/market_data.h"
#include <map>
#include <memory>
#include <string>
#include "../data_storage/real_time_data/real_time_db.h"
#include "../data_storage/storage_manager.h"

class MarketDataHandler : public IMarketDataHandler {
public:
    MarketDataHandler();
    ~MarketDataHandler();

    bool connect(const std::string& exchange_name) override;
    void disconnect(const std::string& exchange_name) override;
    void subscribe(const std::string& symbol) override;
    void unsubscribe(const std::string& symbol) override;

    void setMarketDataCallback(const std::function<void(const MarketData&)>& callback);

private:
    void onExchangeData(const std::string& exchange_name, const std::string& data);

    std::map<std::string, std::shared_ptr<IExchangeAdapter>> exchange_adapters_;
    std::function<void(const MarketData&)> market_data_callback_;
    std::shared_ptr<StorageManager> storage_manager_;  // Assuming StorageManager is the class responsible for storing data
    std::shared_ptr<RealTimeDB> real_time_db_;         // If you have a caching system like a real-time database
    // std::function<void(const MarketData&)> market_data_callback_;  
};

#endif // MARKET_DATA_HANDLER_H
