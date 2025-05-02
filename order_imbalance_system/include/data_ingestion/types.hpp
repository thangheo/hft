#ifndef DATA_INGESTION_TYPES_HPP
#define DATA_INGESTION_TYPES_HPP

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>
#include <cstdint>
#include <memory>

namespace data_ingestion {

using Timestamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

// Exchange identifier
enum class ExchangeId {
    BINANCE,
    FTX,
    COINBASE,
    UNKNOWN
};

// Trading pair (e.g., BTC/USDT)
struct Symbol {
    std::string base;    // Base currency (e.g., BTC)
    std::string quote;   // Quote currency (e.g., USDT)
    
    std::string toString() const {
        return base + "/" + quote;
    }
    
    bool operator==(const Symbol& other) const {
        return base == other.base && quote == other.quote;
    }
    
    bool operator<(const Symbol& other) const {
        if (base != other.base) return base < other.base;
        return quote < other.quote;
    }
};

// Price level in order book
struct PriceLevel {
    double price;
    double quantity;
    std::vector<std::pair<std::string, double>> orders; // Optional order IDs and quantities
    
    bool operator==(const PriceLevel& other) const {
        return price == other.price;
    }
    
    bool operator<(const PriceLevel& other) const {
        return price < other.price;
    }
};

// Update type for order book
enum class UpdateType {
    SNAPSHOT,
    UPDATE,
    TRADE,
    TICKER
};

// Connection status
enum class ConnectionStatus {
    CONNECTED,
    CONNECTING,
    DISCONNECTED,
    RECONNECTING,
    ERROR
};

} // namespace data_ingestion

#endif // DATA_INGESTION_TYPES_HPP