// serializer.h

#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <string>
#include "market_data.pb.h"
#include "order_data.pb.h"
#include "src/common/market_data.h"
#include "src/common/order_data.h"

class Serializer {
public:
    static bool serializeMarketData(const MarketData& data, std::string& output);
    static bool deserializeMarketData(const std::string& input, MarketData& data);

    static bool serializeOrder(const Order& order, std::string& output);
    static bool deserializeOrder(const std::string& input, Order& order);
};

#endif // SERIALIZER_H
