// include/data_ingestion/order_book_types.hpp
#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <atomic>

namespace trading {
namespace data_ingestion {

// Forward declarations
class OrderBook;

// Represents a price level in the order book
struct PriceLevel {
    std::string price;       // Using string to preserve precision
    std::string quantity;    // Using string to preserve precision
    std::vector<std::pair<std::string, std::string>> orders;  // order_id -> quantity pairs
    
    PriceLevel(const std::string& p = "", const std::string& q = "")
        : price(p), quantity(q) {}
};

// Raw market data from an exchange
struct MarketDataUpdate {
    std::string exchange_id;
    std::string trading_pair;
    std::string update_type;  // "orderbook", "trade", "ticker"
    int64_t exchange_timestamp;
    int64_t received_timestamp;
    std::vector<uint8_t> data;  // Raw data from exchange
};

// Normalized order book update
struct OrderBookUpdate {
    std::string exchange_id;
    std::string trading_pair;
    int64_t exchange_timestamp;
    int64_t received_timestamp;
    int64_t sequence_number;
    bool is_snapshot;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    std::string checksum;  // For data integrity validation
};

// Callback types
using OrderBookCallback = std::function<void(const OrderBookUpdate&)>;
using NormalizedOrderBookCallback = std::function<void(const OrderBookUpdate&)>;

// Statistics for publishers
struct PublisherStats {
    std::atomic<uint64_t> messages_published{0};
    std::atomic<uint64_t> messages_dropped{0};
    std::atomic<uint64_t> current_queue_size{0};
    std::atomic<uint64_t> max_queue_size_reached{0};
    std::atomic<uint64_t> bytes_published{0};
    std::atomic<uint64_t> bytes_before_compression{0};
    std::atomic<uint64_t> bytes_after_compression{0};
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point last_publish_time;
};

// Compression options
enum class CompressionAlgorithm {
    NONE,
    SNAPPY,
    ZSTD,
    LZ4
};

// Publisher configuration
struct PublisherConfig {
    std::string zmq_endpoint = "tcp://*:5555";
    int zmq_hwm = 1000;
    int zmq_linger_ms = 1000;
    size_t max_queue_size = 10000;
    int max_publish_rate = 0;  // 0 means no limit
    CompressionAlgorithm compression = CompressionAlgorithm::NONE;
    int compression_level = 3;
    int heartbeat_interval_ms = 5000;
};

} // namespace data_ingestion
} // namespace trading
