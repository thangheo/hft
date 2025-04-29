#ifndef DATA_EXPORTER_H
#define DATA_EXPORTER_H

#include <string>
#include "../common/market_data.h"

class DataExporter {
public:
    DataExporter();
    ~DataExporter();

    bool exportMarketDataToCSV(const std::vector<MarketData>& data, const std::string& filename);
    // Additional methods for other formats
};

#endif // DATA_EXPORTER_H
