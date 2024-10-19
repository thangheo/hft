#ifndef EXCHANGE_INTERFACE_H
#define EXCHANGE_INTERFACE_H

#include <string>
#include <functional>
#include <memory>
#include "../../common/order_data.h"

class IExchangeInterface {
public:
    virtual ~IExchangeInterface() = default;

    /**
     * @brief Initialize the exchange interface.
     * @return True if initialization is successful.
     */
    virtual bool initialize() = 0;

    /**
     * @brief Send an order to the exchange.
     * @param order The order to send.
     * @return True if the order is accepted for processing.
     */
    virtual bool sendOrder(Order& order) = 0;

    /**
     * @brief Set the callback function for order updates.
     * @param callback The function to call with order updates.
     */
    virtual void setOrderUpdateCallback(const std::function<void(const Order&)>& callback) = 0;

    /**
     * @brief Shutdown the exchange interface.
     */
    virtual void shutdown() = 0;
};

#endif // EXCHANGE_INTERFACE_H
