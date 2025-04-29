// candle_data_fetcher.h

#ifndef CANDLE_DATA_FETCHER_H
#define CANDLE_DATA_FETCHER_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

struct Candle {
    uint64_t open_time;     // Open time in milliseconds
    double open;
    double high;
    double low;
    double close;
    double volume;
    uint64_t close_time;    // Close time in milliseconds
    double quote_asset_volume;
    uint64_t number_of_trades;
    double taker_buy_base_asset_volume;
    double taker_buy_quote_asset_volume;
};

class CandleDataFetcher {
public:
    CandleDataFetcher(const std::string& symbol);
    ~CandleDataFetcher();

    void start();
    void stop();

    /**
     * @brief Get the current list of candles.
     * @return A copy of the current candle data.
     */
    std::vector<Candle> getCandleData();

private:
    void fetchHistoricalData();
    void startWebSocket();
    void websocketLoop();
    void handleWebSocketMessage(const std::string& message);

    std::string symbol_;
    std::vector<Candle> candles_;
    std::mutex candles_mutex_;

    // WebSocket variables
    std::thread websocket_thread_;
    std::atomic<bool> running_;

    // WebSocket client
    // You can define your WebSocket client here (e.g., websocketpp client)
};

#endif // CANDLE_DATA_FETCHER_H
