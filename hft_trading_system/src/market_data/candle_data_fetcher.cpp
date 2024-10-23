// candle_data_fetcher.cpp

#include "candle_data_fetcher.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// Include WebSocket headers
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <functional>
#include <sstream>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
namespace websocketpp {
    using lib::placeholders::_1;
    using lib::placeholders::_2;
    using lib::bind;
}

using json = nlohmann::json;

CandleDataFetcher::CandleDataFetcher(const std::string& symbol)
    : symbol_(symbol), running_(false) {
}

CandleDataFetcher::~CandleDataFetcher() {
    stop();
}

void CandleDataFetcher::start() {
    fetchHistoricalData();

    running_ = true;
    websocket_thread_ = std::thread(&CandleDataFetcher::websocketLoop, this);
}

void CandleDataFetcher::stop() {
    running_ = false;
    if (websocket_thread_.joinable()) {
        websocket_thread_.join();
    }
}

std::vector<Candle> CandleDataFetcher::getCandleData() {
    std::lock_guard<std::mutex> lock(candles_mutex_);
    return candles_;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void CandleDataFetcher::fetchHistoricalData() {
    // Build the URL for fetching historical data
    std::string url = "https://api.binance.com/api/v3/klines?symbol=" + symbol_ +
                      "&interval=1m&limit=1000";

    // Initialize CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "CURL initialization failed!" << std::endl;
        return;
    }

    std::string readBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Set up the write callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "CURL request failed: " << curl_easy_strerror(res) << std::endl;
    } else {
        // Parse the JSON response
        try {
            auto json_data = json::parse(readBuffer);

            std::lock_guard<std::mutex> lock(candles_mutex_);
            candles_.clear();

            for (const auto& item : json_data) {
                Candle candle;
                candle.open_time = item[0].get<uint64_t>();
                candle.open = std::stod(item[1].get<std::string>());
                candle.high = std::stod(item[2].get<std::string>());
                candle.low = std::stod(item[3].get<std::string>());
                candle.close = std::stod(item[4].get<std::string>());
                candle.volume = std::stod(item[5].get<std::string>());
                candle.close_time = item[6].get<uint64_t>();
                candle.quote_asset_volume = std::stod(item[7].get<std::string>());
                candle.number_of_trades = item[8].get<uint64_t>();
                candle.taker_buy_base_asset_volume = std::stod(item[9].get<std::string>());
                candle.taker_buy_quote_asset_volume = std::stod(item[10].get<std::string>());

                candles_.push_back(candle);
            }
        } catch (const std::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
        }
    }

    // Clean up CURL
    curl_easy_cleanup(curl);
}

void CandleDataFetcher::websocketLoop() {
    while (running_) {
        try {
            client ws_client_;

            ws_client_.init_asio();

            ws_client_.set_tls_init_handler([](websocketpp::connection_hdl) {
                return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
            });

            ws_client_.set_message_handler(websocketpp::lib::bind(
                &CandleDataFetcher::handleWebSocketMessage, this, websocketpp::lib::placeholders::_2));

            ws_client_.set_fail_handler([this](websocketpp::connection_hdl) {
                std::cerr << "WebSocket connection failed" << std::endl;
                running_ = false;
            });

            ws_client_.set_close_handler([this](websocketpp::connection_hdl) {
                std::cerr << "WebSocket connection closed" << std::endl;
                running_ = false;
            });

            // Build the WebSocket URL
            std::string ws_url = "wss://stream.binance.com:9443/ws/" +
                                 symbol_ + "@kline_1m";

            websocketpp::lib::error_code ec;
            client::connection_ptr con = ws_client_.get_connection(ws_url, ec);

            if (ec) {
                std::cerr << "Could not create connection: " << ec.message() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }

            ws_client_.connect(con);

            ws_client_.run();
        } catch (const std::exception& e) {
            std::cerr << "WebSocket exception: " << e.what() << std::endl;
            // Reconnect after a delay
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void CandleDataFetcher::handleWebSocketMessage(const std::string& message) {
    try {
        auto json_data = json::parse(message);
        if (json_data.contains("k")) {
            auto kline = json_data["k"];

            Candle candle;
            candle.open_time = kline["t"].get<uint64_t>();
            candle.open = std::stod(kline["o"].get<std::string>());
            candle.high = std::stod(kline["h"].get<std::string>());
            candle.low = std::stod(kline["l"].get<std::string>());
            candle.close = std::stod(kline["c"].get<std::string>());
            candle.volume = std::stod(kline["v"].get<std::string>());
            candle.close_time = kline["T"].get<uint64_t>();
            candle.quote_asset_volume = std::stod(kline["q"].get<std::string>());
            candle.number_of_trades = kline["n"].get<uint64_t>();
            candle.taker_buy_base_asset_volume = std::stod(kline["V"].get<std::string>());
            candle.taker_buy_quote_asset_volume = std::stod(kline["Q"].get<std::string>());

            bool is_closed = kline["x"].get<bool>();

            std::lock_guard<std::mutex> lock(candles_mutex_);

            if (!candles_.empty() && candles_.back().open_time == candle.open_time) {
                // Update existing candle
                candles_.back() = candle;
            } else {
                // Add new candle
                candles_.push_back(candle);
                if (candles_.size() > 1000) {
                    candles_.erase(candles_.begin()); // Remove oldest candle
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling WebSocket message: " << e.what() << std::endl;
    }
}
