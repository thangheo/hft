// data_api_server.cpp

#include "httplib.h"
#include "src/data_storage/historical_data/historical_db.h"
#include "src/data_storage/serialization/serializer.h"
#include "src/common/market_data.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    httplib::Server svr;

    HistoricalDB historical_db;
    if (!historical_db.connect("dbname=hft user=postgres password=secret")) {
        std::cerr << "Failed to connect to Historical DB" << std::endl;
        return 1;
    }

    svr.Get("/market_data", [&](const httplib::Request& req, httplib::Response& res) {
        // Parse query parameters
        auto symbol = req.get_param_value("symbol");
        auto start_time_str = req.get_param_value("start_time");
        auto end_time_str = req.get_param_value("end_time");

        // Convert times to chrono
        std::chrono::system_clock::time_point start_time, end_time;
        // Parse start_time_str and end_time_str to time_point

        // Query the database
        auto data = historical_db.queryMarketData(symbol, start_time, end_time);

        // Convert results to JSON
        json j;
        for (const auto& md : data) {
            json md_json;
            md_json["exchange"] = md.exchange;
            md_json["symbol"] = md.symbol;
            md_json["timestamp"] = std::chrono::system_clock::to_time_t(md.timestamp);
            // Add bids and asks
            j.push_back(md_json);
        }

        res.set_content(j.dump(), "application/json");
    });

    svr.listen("0.0.0.0", 8080);

    return 0;
}
