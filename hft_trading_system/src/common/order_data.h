// order_data.h

#ifndef ORDER_DATA_H
#define ORDER_DATA_H

#include <string>
#include <chrono>

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED
};

struct Order {
    std::string order_id;
    std::string client_order_id;
    std::string symbol;
    std::string side; // BUY or SELL
    double price;
    double quantity;
    OrderStatus status;
    std::chrono::system_clock::time_point timestamp;
    // Additional fields as needed
};

#endif // ORDER_DATA_H
