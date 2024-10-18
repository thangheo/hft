// real_time_db.cpp

#include "real_time_db.h"
#include "../serialization/serializer.h"
#include <iostream>

RealTimeDB::RealTimeDB() {}

RealTimeDB::~RealTimeDB() {
    disconnect();
}

bool RealTimeDB::connect(const std::string& host, int port) {
    try {
        sw::redis::ConnectionOptions connection_options;
        connection_options.host = host;
        connection_options.port = port;

        redis_ = std::make_shared<sw::redis::Redis>(connection_options);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis connection error: " << e.what() << std::endl;
        return false;
    }
}

void RealTimeDB::disconnect() {
    redis_.reset();
}

bool RealTimeDB::storeMarketData(const std::string& key, const std::string& data) {
    if (!redis_) return false;
    try {
        redis_->set(key, data);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis set error: " << e.what() << std::endl;
        return false;
    }
}

bool RealTimeDB::getMarketData(const std::string& key, std::string& data) {
    if (!redis_) return false;
    try {
        auto val = redis_->get(key);
        if (val) {
            data = *val;
            return true;
        } else {
            return false; // Key does not exist
        }
    } catch (const std::exception& e) {
        std::cerr << "Redis get error: " << e.what() << std::endl;
        return false;
    }
}
bool RealTimeDB::cacheMarketData(const MarketData& data) {
    if (!redis_) return false;

    std::string serialized_data;
    if (!Serializer::serializeMarketData(data, serialized_data)) {
        std::cerr << "Failed to serialize market data for caching" << std::endl;
        return false;
    }

    std::string key = "cache:market_data:" + data.symbol;
    try {
        redis_->set(key, serialized_data);
        // Optionally set an expiration time
        redis_->expire(key, std::chrono::seconds(60));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis cache set error: " << e.what() << std::endl;
        return false;
    }
}

bool RealTimeDB::getCachedMarketData(const std::string& symbol, MarketData& data) {
    if (!redis_) return false;

    std::string key = "cache:market_data:" + symbol;
    try {
        auto val = redis_->get(key);
        if (val) {
            if (Serializer::deserializeMarketData(*val, data)) {
                return true;
            } else {
                std::cerr << "Failed to deserialize cached market data" << std::endl;
                return false;
            }
        } else {
            // Data not in cache
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Redis cache get error: " << e.what() << std::endl;
        return false;
    }
}

