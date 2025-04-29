#include "data_exporter.h"
#include <fstream>

bool DataExporter::exportMarketDataToCSV(const std::vector<MarketData>& data, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    // Write header
    file << "exchange,symbol,timestamp,bid_price,bid_quantity,ask_price,ask_quantity\n";

    for (const auto& md : data) {
        for (size_t i = 0; i < md.bids.size(); ++i) {
            file << md.exchange << "," << md.symbol << "," << std::chrono::system_clock::to_time_t(md.timestamp) << ",";
            file << md.bids[i].price << "," << md.bids[i].quantity << ",";
            file << md.asks[i].price << "," << md.asks[i].quantity << "\n";
        }
    }

    file.close();
    return true;
}
