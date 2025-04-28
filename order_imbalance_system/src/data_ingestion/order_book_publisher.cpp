// src/data_ingestion/order_book_publisher.cpp
#include "data_ingestion/order_book_publisher.hpp"
#include "generated/order_book.pb.h"  // Generated Protocol Buffer headers
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

// Include compression libraries based on what's actually available
#ifdef HAS_SNAPPY
#include <snappy.h>
#endif

#ifdef HAS_ZSTD
#include <zstd.h>
#endif

#ifdef HAS_LZ4
#include <lz4.h>
#endif

namespace trading {
namespace data_ingestion {

OrderBookPublisher::OrderBookPublisher(const PublisherConfig& config)
    : config_(config),
      context_(std::make_unique<zmq::context_t>(1)),
      last_rate_check_time_(std::chrono::system_clock::now()) {
    
    stats_.start_time = std::chrono::system_clock::now();
    stats_.last_publish_time = stats_.start_time;
    
    // Initialize ZeroMQ socket
    initializeSocket();
}

OrderBookPublisher::~OrderBookPublisher() {
    stop();
}

bool OrderBookPublisher::start() {
    if (running_) {
        return false;  // Already running
    }
    
    running_ = true;
    worker_thread_ = std::thread(&OrderBookPublisher::workerFunction, this);
    
    return true;
}

void OrderBookPublisher::stop() {
    if (!running_) {
        return;  // Already stopped
    }
    
    running_ = false;
    
    // Notify worker thread to check running_ status
    queue_condition_.notify_all();
    
    // Join worker thread if joinable
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    // Clear the queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        std::queue<std::pair<std::string, std::vector<uint8_t>>> empty;
        std::swap(message_queue_, empty);
    }
}

bool OrderBookPublisher::publishOrderBookUpdate(const OrderBookUpdate& update) {
    if (!running_) {
        return false;
    }
    
    // Apply rate limiting if configured
    if (shouldThrottlePublishing()) {
        // Invoke back pressure callback if registered
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (back_pressure_callback_) {
            back_pressure_callback_(update);
        }
        
        stats_.messages_dropped++;
        return false;
    }
    
    // Create topic string
    std::string topic = createTopic(update);
    
    // Serialize to Protocol Buffers
    std::vector<uint8_t> data = serializeOrderBookUpdate(update);
    
    // Update statistics before compression
    stats_.bytes_before_compression += data.size();
    
    // Apply compression if configured
    if (config_.compression != CompressionAlgorithm::NONE) {
        data = compressData(data);
    }
    
    // Update statistics after compression
    stats_.bytes_after_compression += data.size();
    
    // Add to publish queue with back-pressure checks
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        // Check queue size for back-pressure
        if (message_queue_.size() >= config_.max_queue_size) {
            // Apply back pressure - don't queue the message
            std::lock_guard<std::mutex> cb_lock(callback_mutex_);
            if (back_pressure_callback_) {
                back_pressure_callback_(update);
            }
            
            stats_.messages_dropped++;
            return false;
        }
        
        // Queue the message
        message_queue_.push(std::make_pair(topic, std::move(data)));
        stats_.current_queue_size = message_queue_.size();
        
        if (stats_.current_queue_size > stats_.max_queue_size_reached) {
            stats_.max_queue_size_reached = stats_.current_queue_size;
        }
    }
    
    // Signal worker thread that a new message is available
    queue_condition_.notify_one();
    
    return true;
}

PublisherStats OrderBookPublisher::getStats() const {
    return stats_;
}

void OrderBookPublisher::registerBackPressureCallback(std::function<void(const OrderBookUpdate&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    back_pressure_callback_ = std::move(callback);
}

bool OrderBookPublisher::updateConfig(const PublisherConfig& config) {
    // We can't change the ZMQ endpoint while running
    if (running_ && config_.zmq_endpoint != config.zmq_endpoint) {
        return false;
    }
    
    config_ = config;
    
    // If we're not running, update the socket with new config
    if (!running_) {
        initializeSocket();
    }
    
    return true;
}

void OrderBookPublisher::workerFunction() {
    auto last_heartbeat_time = std::chrono::system_clock::now();
    
    while (running_) {
        std::pair<std::string, std::vector<uint8_t>> message;
        bool has_message = false;
        
        // Get a message from the queue
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Wait for a message or check running_ status every 100ms
            auto wait_result = queue_condition_.wait_for(lock, std::chrono::milliseconds(100), 
                [this] { return !message_queue_.empty() || !running_; });
            
            if (!running_) {
                break;
            }
            
            // Send heartbeat if needed
            auto now = std::chrono::system_clock::now();
            if (config_.heartbeat_interval_ms > 0 && 
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat_time).count() 
                    >= config_.heartbeat_interval_ms) {
                sendHeartbeat();
                last_heartbeat_time = now;
            }
            
            // If there's no message, continue the loop
            if (message_queue_.empty()) {
                continue;
            }
            
            // Get the message
            message = std::move(message_queue_.front());
            message_queue_.pop();
            has_message = true;
            
            // Update queue size stats
            stats_.current_queue_size = message_queue_.size();
        }
        
        // If we have a message, publish it
        if (has_message) {
            try {
                // First send the topic
                zmq::message_t topic_msg(message.first.data(), message.first.size());
                publisher_->send(topic_msg, zmq::send_flags::sndmore);
                
                // Then send the data
                zmq::message_t data_msg(message.second.data(), message.second.size());
                publisher_->send(data_msg, zmq::send_flags::none);
                
                // Update statistics
                stats_.messages_published++;
                stats_.bytes_published += message.first.size() + message.second.size();
                stats_.last_publish_time = std::chrono::system_clock::now();
            }
            catch (const zmq::error_t& e) {
                std::cerr << "ZeroMQ error when publishing: " << e.what() << std::endl;
                // We could add retry logic here if needed
            }
        }
    }
}

std::vector<uint8_t> OrderBookPublisher::serializeOrderBookUpdate(const OrderBookUpdate& update) {
    // Convert our internal OrderBookUpdate to protobuf message
    proto::OrderBookUpdate proto_update;
    
    // Fill in the fields
    proto_update.set_exchange_id(update.exchange_id);
    proto_update.set_trading_pair(update.trading_pair);
    proto_update.set_exchange_timestamp(update.exchange_timestamp);
    proto_update.set_received_timestamp(update.received_timestamp);
    proto_update.set_sequence_number(update.sequence_number);
    proto_update.set_is_snapshot(update.is_snapshot);
    proto_update.set_checksum(update.checksum);
    
    // Add bids
    for (const auto& bid : update.bids) {
        auto* proto_bid = proto_update.add_bids();
        proto_bid->set_price(bid.price);
        proto_bid->set_quantity(bid.quantity);
        
        // Add orders if available
        for (const auto& order : bid.orders) {
            auto* proto_order = proto_bid->add_orders();
            proto_order->set_order_id(order.first);
            proto_order->set_quantity(order.second);
        }
    }
    
    // Add asks
    for (const auto& ask : update.asks) {
        auto* proto_ask = proto_update.add_asks();
        proto_ask->set_price(ask.price);
        proto_ask->set_quantity(ask.quantity);
        
        // Add orders if available
        for (const auto& order : ask.orders) {
            auto* proto_order = proto_ask->add_orders();
            proto_order->set_order_id(order.first);
            proto_order->set_quantity(order.second);
        }
    }
    
    // Serialize to binary
    std::string serialized;
    if (!proto_update.SerializeToString(&serialized)) {
        std::cerr << "Failed to serialize order book update" << std::endl;
        return {};
    }
    
    return std::vector<uint8_t>(serialized.begin(), serialized.end());
}

std::vector<uint8_t> OrderBookPublisher::compressData(const std::vector<uint8_t>& data) {
    switch (config_.compression) {
        case CompressionAlgorithm::SNAPPY:
            #ifdef HAS_SNAPPY
            {
                std::string compressed;
                snappy::Compress(reinterpret_cast<const char*>(data.data()), data.size(), &compressed);
                return std::vector<uint8_t>(compressed.begin(), compressed.end());
            }
            #else
            std::cerr << "Snappy compression requested but not available" << std::endl;
            #endif
            break;
            
        case CompressionAlgorithm::ZSTD:
            #ifdef HAS_ZSTD
            {
                // Allocate buffer for compressed data (worst case size)
                size_t compressed_size = ZSTD_compressBound(data.size());
                std::vector<uint8_t> compressed(compressed_size);
                
                // Compress
                compressed_size = ZSTD_compress(
                    compressed.data(), compressed.size(),
                    data.data(), data.size(),
                    config_.compression_level);
                
                if (ZSTD_isError(compressed_size)) {
                    std::cerr << "ZSTD compression error: " 
                              << ZSTD_getErrorName(compressed_size) << std::endl;
                    return data;  // Return uncompressed on error
                }
                
                // Resize to actual compressed size
                compressed.resize(compressed_size);
                return compressed;
            }
            #else
            std::cerr << "ZSTD compression requested but not available" << std::endl;
            #endif
            break;
            
        case CompressionAlgorithm::LZ4:
            #ifdef HAS_LZ4
            {
                // Estimate compressed size
                int max_compressed_size = LZ4_compressBound(data.size());
                std::vector<uint8_t> compressed(max_compressed_size);
                
                // Compress
                int compressed_size = LZ4_compress_default(
                    reinterpret_cast<const char*>(data.data()), 
                    reinterpret_cast<char*>(compressed.data()),
                    data.size(), max_compressed_size);
                
                if (compressed_size <= 0) {
                    std::cerr << "LZ4 compression error" << std::endl;
                    return data;  // Return uncompressed on error
                }
                
                // Resize to actual compressed size
                compressed.resize(compressed_size);
                return compressed;
            }
            #else
            std::cerr << "LZ4 compression requested but not available" << std::endl;
            #endif
            break;
            
        case CompressionAlgorithm::NONE:
        default:
            break;
    }
    
    // Default: return uncompressed data
    return data;
}

std::string OrderBookPublisher::createTopic(const OrderBookUpdate& update) {
    // Format: market.orderbook.normalized.<exchange_id>.<trading_pair>
    std::stringstream ss;
    ss << "market.orderbook.normalized." 
       << update.exchange_id << "." 
       << update.trading_pair;
    
    return ss.str();
}

void OrderBookPublisher::sendHeartbeat() {
    try {
        // Topic for heartbeat
        std::string topic = "system.heartbeat.orderbook_publisher";
        zmq::message_t topic_msg(topic.data(), topic.size());
        publisher_->send(topic_msg, zmq::send_flags::sndmore);
        
        // Create heartbeat message with timestamp
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        std::string msg = "timestamp=" + std::to_string(now_ms);
        zmq::message_t data_msg(msg.data(), msg.size());
        publisher_->send(data_msg, zmq::send_flags::none);
    }
    catch (const zmq::error_t& e) {
        std::cerr << "ZeroMQ error when sending heartbeat: " << e.what() << std::endl;
    }
}

bool OrderBookPublisher::shouldThrottlePublishing() {
    // If no rate limit, never throttle
    if (config_.max_publish_rate <= 0) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_rate_check_time_).count();
    
    // Reset counter every second
    if (elapsed_ms >= 1000) {
        messages_in_current_interval_ = 0;
        last_rate_check_time_ = now;
        return false;
    }
    
    // Check if we've exceeded the rate
    if (messages_in_current_interval_ >= config_.max_publish_rate) {
        return true;  // Throttle
    }
    
    // Increment counter and don't throttle
    messages_in_current_interval_++;
    return false;
}

void OrderBookPublisher::initializeSocket() {
    try {
        // Create new socket
        publisher_ = std::make_unique<zmq::socket_t>(*context_, zmq::socket_type::pub);
        
        // Set socket options
        publisher_->set(zmq::sockopt::linger, config_.zmq_linger_ms);
        if (config_.zmq_hwm > 0) {
            publisher_->set(zmq::sockopt::sndhwm, config_.zmq_hwm);
        }
        
        // Bind to endpoint
        publisher_->bind(config_.zmq_endpoint);
    }
    catch (const zmq::error_t& e) {
        std::cerr << "ZeroMQ error when initializing socket: " << e.what() << std::endl;
        throw;  // Re-throw the exception
    }
}

} // namespace data_ingestion
} // namespace trading
