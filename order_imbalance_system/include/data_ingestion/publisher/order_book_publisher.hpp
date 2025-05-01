// include/data_ingestion/order_book_publisher.hpp
#pragma once

#include "order_book.hpp"
#include <zmq.hpp>
#include <string>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <unordered_map>
#include <functional>

namespace trading {
namespace data_ingestion {

// Compression algorithms supported by the publisher
enum class CompressionAlgorithm {
    NONE,       // No compression
    SNAPPY,     // Google's Snappy compression
    ZSTD,       // Facebook's Zstandard compression
    LZ4         // LZ4 compression
};

// Configuration for the publisher
struct PublisherConfig {
    // Base ZeroMQ endpoint (e.g., "tcp://*:5555")
    std::string zmq_endpoint = "tcp://*:5555";
    
    // Maximum queue size before applying back pressure
    size_t max_queue_size = 1000;
    
    // Maximum publishing rate in messages per second (0 for unlimited)
    int max_publish_rate = 0;
    
    // Compression algorithm to use
    CompressionAlgorithm compression = CompressionAlgorithm::NONE;
    
    // Compression level (1-9, higher means more compression but slower)
    int compression_level = 3;
    
    // How often to send heartbeat messages (in milliseconds, 0 to disable)
    int heartbeat_interval_ms = 5000;
    
    // High water mark for ZeroMQ socket (0 for unlimited)
    int zmq_hwm = 1000;
    
    // ZeroMQ linger period when closing socket (milliseconds)
    int zmq_linger_ms = 1000;
};

// Statistics for monitoring publisher performance
struct PublisherStats {
    std::atomic<uint64_t> messages_published{0};
    std::atomic<uint64_t> messages_dropped{0};
    std::atomic<uint64_t> current_queue_size{0};
    std::atomic<uint64_t> max_queue_size_reached{0};
    std::atomic<uint64_t> bytes_published{0};
    std::atomic<uint64_t> bytes_before_compression{0};
    std::atomic<uint64_t> bytes_after_compression{0};
    std::chrono::system_clock::time_point last_publish_time;
    std::chrono::system_clock::time_point start_time;
};

class OrderBookPublisher {
public:
    // Constructor with configuration
    explicit OrderBookPublisher(const PublisherConfig& config = PublisherConfig());
    
    // Destructor
    ~OrderBookPublisher();
    
    // Start the publisher thread
    bool start();
    
    // Stop the publisher thread
    void stop();
    
    // Publish an order book update
    // Returns true if the message was queued, false if back pressure was applied
    bool publishOrderBookUpdate(const OrderBookUpdate& update);
    
    // Get current publisher statistics
    PublisherStats getStats() const;
    
    // Register a callback for when back pressure is applied
    void registerBackPressureCallback(std::function<void(const OrderBookUpdate&)> callback);
    
    // Update publisher configuration
    bool updateConfig(const PublisherConfig& config);
    
private:
    // Configuration
    PublisherConfig config_;
    
    // ZeroMQ context and publisher socket
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> publisher_;
    
    // Worker thread for asynchronous publishing
    std::thread worker_thread_;
    
    // Message queue and related synchronization
    std::queue<std::pair<std::string, std::vector<uint8_t>>> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::atomic<bool> running_{false};
    
    // Back pressure callback
    std::function<void(const OrderBookUpdate&)> back_pressure_callback_;
    std::mutex callback_mutex_;
    
    // Rate limiting
    std::chrono::system_clock::time_point last_rate_check_time_;
    int messages_in_current_interval_{0};
    
    // Statistics
    PublisherStats stats_;
    
    // Worker thread function
    void workerFunction();
    
    // Serialize OrderBookUpdate to Protocol Buffers
    std::vector<uint8_t> serializeOrderBookUpdate(const OrderBookUpdate& update);
    
    // Compress data using selected algorithm
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);
    
    // Create topic string for an order book update
    std::string createTopic(const OrderBookUpdate& update);
    
    // Send heartbeat message
    void sendHeartbeat();
    
    // Apply rate limiting
    bool shouldThrottlePublishing();
    
    // Initialize ZeroMQ socket
    void initializeSocket();
};

} // namespace data_ingestion
} // namespace trading
