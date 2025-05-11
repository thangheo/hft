#ifndef DATA_INGESTION_TYPES_HPP
#define DATA_INGESTION_TYPES_HPP

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>
#include <cstdint>
#include <memory>
#include <functional>
#include <unordered_map>

namespace trading {
namespace data_ingestion {

using Timestamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
/**
 * @struct ExchangeData
 * @brief Represents raw data received from an exchange
 */
struct ExchangeData {
    std::string exchangeId;        // Identifier for the exchange (e.g., "binance")
    std::string symbol;            // Trading pair symbol (e.g., "BTCUSDT")
    std::string data;              // Raw data received (e.g., JSON string)
    uint64_t receivedTime;         // Timestamp when data was received
    std::string dataType;          // Type of data (e.g., "orderbook", "trade")
    
    // Other potentially useful fields
    bool isSnapshot{false};        // Whether this is a full snapshot or an update
    std::string sourceType;        // "websocket" or "rest"
};
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
//simple PriceLevel with price and quantity
struct PriceLevelBase {
    double price;     // Price level
    double quantity;  // Quantity at this price level
    
    PriceLevelBase(double p = 0.0, double q = 0.0) : price(p), quantity(q) {}
    
    bool operator==(const PriceLevelBase& other) const {
        return price == other.price && quantity == other.quantity;
    }
    
    bool operator<(const PriceLevelBase& other) const {
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

// Order book update
// class simple_PriceLevel {
//     public:
//         double price;     // Add this member
//         double quantity;  // Add this member
        
//         // Constructor
//         simple_PriceLevel(double p = 0.0, double q = 0.0) : price(p), quantity(q) {}
//     };
    

} // namespace data_ingestion
} // namespace trading
#endif // DATA_INGESTION_TYPES_HPP