// include/data_ingestion/publisher.hpp
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

namespace trading {
namespace data_ingestion {

class Publisher {
public:
    Publisher(const std::string& zmq_address);
    ~Publisher();
    
    // Start the publisher in a separate thread
    void start();
    
    // Stop the publisher thread
    void stop();
    
    // Publish an order book update
    void publishOrderBookUpdate(const OrderBookUpdate& update);
    
    // Publish raw market data
    void publishRawMarketData(const MarketDataUpdate& update);
    
private:
    std::string zmq_address_;
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> publisher_;
    
    std::thread worker_thread_;
    std::queue<std::vector<uint8_t>> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> running_;
    
    // Worker thread function
    void workerFunction();
    
    // Serialize OrderBookUpdate to Protocol Buffers
    std::vector<uint8_t> serializeOrderBookUpdate(const OrderBookUpdate& update);
    
    // Serialize MarketDataUpdate to Protocol Buffers
    std::vector<uint8_t> serializeMarketDataUpdate(const MarketDataUpdate& update);
};

} // namespace data_ingestion
} // namespace trading
