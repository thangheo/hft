// imarket_data_handler.h

#ifndef IMARKET_DATA_HANDLER_H
#define IMARKET_DATA_HANDLER_H

#include <string>
#include <memory>

class IMarketDataHandler {
public:
    virtual ~IMarketDataHandler() = default;

    /**
     * @brief Connects to the exchange and starts receiving market data.
     * @param exchange_name The name of the exchange to connect to.
     * @return True if the connection was successful, false otherwise.
     */
    virtual bool connect(const std::string& exchange_name) = 0;

    /**
     * @brief Disconnects from the exchange and stops receiving market data.
     * @param exchange_name The name of the exchange to disconnect from.
     */
    virtual void disconnect(const std::string& exchange_name) = 0;

    /**
     * @brief Subscribes to a market data stream.
     * @param symbol The trading pair or symbol to subscribe to.
     */
    virtual void subscribe(const std::string& symbol) = 0;

    /**
     * @brief Unsubscribes from a market data stream.
     * @param symbol The trading pair or symbol to unsubscribe from.
     */
    virtual void unsubscribe(const std::string& symbol) = 0;
};

#endif // IMARKET_DATA_HANDLER_H
