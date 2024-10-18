// serializer.cpp

#include "serializer.h"
#include <google/protobuf/util/time_util.h>

using namespace google::protobuf::util;

bool Serializer::serializeMarketData(const MarketData& data, std::string& output) {
    hft::MarketData pb_data;
    pb_data.set_exchange(data.exchange);
    pb_data.set_symbol(data.symbol);
    pb_data.set_timestamp(TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(data.timestamp)).seconds() * 1000);

    for (const auto& bid : data.bids) {
        hft::OrderBookEntry* pb_bid = pb_data.add_bids();
        pb_bid->set_price(bid.price);
        pb_bid->set_quantity(bid.quantity);
    }

    for (const auto& ask : data.asks) {
        hft::OrderBookEntry* pb_ask = pb_data.add_asks();
        pb_ask->set_price(ask.price);
        pb_ask->set_quantity(ask.quantity);
    }

    return pb_data.SerializeToString(&output);
}

bool Serializer::deserializeMarketData(const std::string& input, MarketData& data) {
    hft::MarketData pb_data;
    if (!pb_data.ParseFromString(input)) {
        return false;
    }

    data.exchange = pb_data.exchange();
    data.symbol = pb_data.symbol();
    data.timestamp = std::chrono::system_clock::from_time_t(pb_data.timestamp() / 1000);

    for (const auto& pb_bid : pb_data.bids()) {
        data.bids.push_back({pb_bid.price(), pb_bid.quantity()});
    }

    for (const auto& pb_ask : pb_data.asks()) {
        data.asks.push_back({pb_ask.price(), pb_ask.quantity()});
    }

    return true;
}

bool Serializer::serializeOrder(const Order& order, std::string& output) {
    hft::Order pb_order;
    pb_order.set_order_id(order.order_id);
    pb_order.set_client_order_id(order.client_order_id);
    pb_order.set_symbol(order.symbol);
    pb_order.set_side(order.side);
    pb_order.set_price(order.price);
    pb_order.set_quantity(order.quantity);
    pb_order.set_status(static_cast<hft::OrderStatus>(order.status));
    pb_order.set_timestamp(TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(order.timestamp)).seconds() * 1000);

    return pb_order.SerializeToString(&output);
}

bool Serializer::deserializeOrder(const std::string& input, Order& order) {
    hft::Order pb_order;
    if (!pb_order.ParseFromString(input)) {
        return false;
    }

    order.order_id = pb_order.order_id();
    order.client_order_id = pb_order.client_order_id();
    order.symbol = pb_order.symbol();
    order.side = pb_order.side();
    order.price = pb_order.price();
    order.quantity = pb_order.quantity();
    order.status = static_cast<OrderStatus>(pb_order.status());
    order.timestamp = std::chrono::system_clock::from_time_t(pb_order.timestamp() / 1000);

    return true;
}
