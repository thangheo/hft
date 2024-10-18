// storage_manager.cpp

#include "storage_manager.h"
#include <iostream>

StorageManager::StorageManager() {}

StorageManager::~StorageManager() {
    shutdown();
}

bool StorageManager::initialize() {
    real_time_db_ = std::make_unique<RealTimeDB>();
    if (!real_time_db_->connect()) {
        std::cerr << "Failed to connect to Real-Time DB" << std::endl;
        return false;
    }

    historical_db_ = std::make_unique<HistoricalDB>();
    if (!historical_db_->connect("dbname=hft user=postgres password=secret")) {
        std::cerr << "Failed to connect to Historical DB" << std::endl;
        return false;
    }

    return true;
}

void StorageManager::shutdown() {
    if (real_time_db_) {
        real_time_db_->disconnect();
    }
    if (historical_db_) {
        historical_db_->disconnect();
    }
}

bool StorageManager::storeRealTimeMarketData(const MarketData& data) {
    std::string serialized_data;
    if (!Serializer::serializeMarketData(data, serialized_data)) {
        std::cerr << "Failed to serialize market data" << std::endl;
        return false;
    }

    std::string key = data.exchange + ":" + data.symbol;
    return real_time_db_->storeMarketData(key, serialized_data);
}

bool StorageManager::storeHistoricalMarketData(const MarketData& data) {
    return historical_db_->storeMarketData(data);
}

bool StorageManager::storeOrder(const Order& order) {
    return historical_db_->storeOrder(order);
}
