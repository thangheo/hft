// adapters/exchange_adapter.h

#ifndef EXCHANGE_ADAPTER_H
#define EXCHANGE_ADAPTER_H

#include <string>
#include <functional>

class IExchangeAdapter {
public:
    virtual ~IExchangeAdapter() = default;

    /**
     * @brief Connects to the exchange's market data feed.
     * @return True if the connection was successful, false otherwise.
     */
    virtual bool connect() = 0;

    /**
     * @brief Disconnects from the exchange's market data feed.
     */
    virtual void disconnect() = 0;

    /**
     * @brief Subscribes to a symbol's market data.
     * @param symbol The symbol to subscribe to.
     */
    virtual void subscribe(const std::string& symbol) = 0;

    /**
     * @brief Unsubscribes from a symbol's market data.
     * @param symbol The symbol to unsubscribe from.
     */
    virtual void unsubscribe(const std::string& symbol) = 0;

    /**
     * @brief Sets the callback function to handle incoming market data.
     * @param callback The function to call with new market data.
     */
    virtual void setDataCallback(const std::function<void(const std::string&)>& callback) = 0;
};

#endif // EXCHANGE_ADAPTER_H
