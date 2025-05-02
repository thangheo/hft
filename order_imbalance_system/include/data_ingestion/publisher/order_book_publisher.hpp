// include/data_ingestion/order_book_publisher.hpp
#pragma once

#include "data_ingestion/orderbook/order_book_types.hpp"
// #include "data_ingestion/publisher/publisher_config.hpp"
#include <zmq.hpp>
#include <string>
#include <memory>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>

namespace trading {
namespace data_ingestion {

// Publisher interface
class Publisher {
public:
    virtual ~Publisher() = default;
    
    // Start the publisher
    virtual bool start() = 0;
    
    // Stop the publisher
    virtual void stop() = 0;
    
    // Publish an update
    virtual bool publish(const std::string& topic, const std::vector<uint8_t>& data) = 0;
    
    // Get statistics
    virtual PublisherStats getStats() const = 0;
};

// ZeroMQ-based publisher
class ZeroMQPublisher : public Publisher {
public:
    ZeroMQPublisher(const PublisherConfig& config);
    ~ZeroMQPublisher() override;
    
    bool start() override;
    void stop() override;
    bool publish(const std::string& topic, const std::vector<uint8_t>& data) override;
    PublisherStats getStats() const override;
    
    // Register a callback for back pressure notification
    void registerBackPressureCallback(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback);
    
    // Update configuration
    bool updateConfig(const PublisherConfig& config);
    
private:
    PublisherConfig config_;
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> publisher_;
    
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
    
    std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::queue<std::pair<std::string, std::vector<uint8_t>>> message_queue_;
    
    std::mutex callback_mutex_;
    std::function<void(const std::string&, const std::vector<uint8_t>&)> back_pressure_callback_;
    
    PublisherStats stats_;
    std::atomic<int> messages_in_current_interval_{0};
    std::chrono::system_clock::time_point last_rate_check_time_;
    
    // Worker thread function
    void workerFunction();
    
    // Helper functions
    void initializeSocket();
    bool shouldThrottlePublishing();
    void sendHeartbeat();
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);
};

// Main order book publisher class
class OrderBookPublisher {
public:
    OrderBookPublisher(const PublisherConfig& config);
    ~OrderBookPublisher();
    
    // Start the publisher
    bool start();
    
    // Stop the publisher
    void stop();
    
    // Publish an order book update
    bool publishOrderBookUpdate(const OrderBookUpdate& update);
    
    // Get statistics
    PublisherStats getStats() const;
    
    // Register a callback for back pressure notification
    void registerBackPressureCallback(std::function<void(const OrderBookUpdate&)> callback);
    
    // Update configuration
    bool updateConfig(const PublisherConfig& config);
    
private:
    PublisherConfig config_;
    std::unique_ptr<ZeroMQPublisher> publisher_;
    std::function<void(const OrderBookUpdate&)> back_pressure_callback_;
    
    // Helper functions
    std::string createTopic(const OrderBookUpdate& update);
    std::vector<uint8_t> serializeOrderBookUpdate(const OrderBookUpdate& update);
};

} // namespace data_ingestion
} // namespace trading
