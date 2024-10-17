// adapters/binance_adapter.h

#ifndef BINANCE_ADAPTER_H
#define BINANCE_ADAPTER_H

#include "exchange_adapter.h"
#include "../protocols/websocket_client.h"
#include <string>
#include <functional>

class BinanceAdapter : public IExchangeAdapter {
public:
    BinanceAdapter();
    ~BinanceAdapter();

    bool connect() override;
    void disconnect() override;
    void subscribe(const std::string& symbol) override;
    void unsubscribe(const std::string& symbol) override;
    void setDataCallback(const std::function<void(const std::string&)>& callback) override;

private:
    WebSocketClient ws_client_;
    std::function<void(const std::string&)> data_callback_;
};

#endif // BINANCE_ADAPTER_H
