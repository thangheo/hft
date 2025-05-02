// src/data_ingestion/main.cpp
#include "data_ingestion/order_book_types.hpp"
#include "data_ingestion/order_book_normalizer.hpp"
#include "data_ingestion/data_quality.hpp"
#include "data_ingestion/order_book_publisher.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

using namespace trading::data_ingestion;

// Global flag for clean shutdown
std::atomic<bool> running(true);

// Signal handler
void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    try {
        // Register signal handler
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        std::cout << "Starting Order Book Normalizer Service" << std::endl;
        
        // Create publisher
        PublisherConfig pub_config;
        pub_config.zmq_endpoint = "tcp://*:5555";
        pub_config.max_queue_size = 10000;
        pub_config.max_publish_rate = 5000;  // 5000 updates per second
        pub_config.compression = CompressionAlgorithm::ZSTD;
        pub_config.compression_level = 3;
        
        auto publisher = std::make_shared<OrderBookPublisher>(pub_config);
        
        // Create data quality control
        auto quality_control = std::make_shared<DataQualityControl>();
        
        // Create normalizer
        auto normalizer = std::make_shared<OrderBookNormalizer>(
            [publisher, quality_control](const OrderBookUpdate& update) {
                // Apply quality checks
                OrderBookUpdate validated_update = update;  // Copy to avoid modifying original
                if (quality_control->validateUpdate(validated_update)) {
                    // Publish the validated update
                    publisher->publishOrderBookUpdate(validated_update);
                } else {
                    std::cerr << "Quality check failed for update: " 
                              << update.exchange_id << ":" << update.trading_pair 
                              << " seq=" << update.sequence_number << std::endl;
                }
            });
        
        // Start the publisher
        if (!publisher->start()) {
            std::cerr << "Failed to start publisher" << std::endl;
            return 1;
        }
        
        std::cout << "Order Book Normalizer Service started" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        // In a real application, we would connect to exchange WebSockets here
        // For demonstration, we'll just simulate some updates
        
        int64_t sequence_number = 1;
        
        // Main service loop
        while (running) {
            // Sleep to avoid high CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Simulate a market data update
            MarketDataUpdate update;
            update.exchange_id = "binance";
            update.trading_pair = "BTCUSDT";
            update.update_type = "orderbook";
            update.exchange_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            update.received_timestamp = update.exchange_timestamp + 5000000;  // 5ms later
            
            // Simulate some JSON data (would come from exchange in real app)
            std::string json = R"({
                "u": )" + std::to_string(sequence_number++) + R"(,
                "s": "BTCUSDT",
                "b": [["45000.00", "1.5"], ["44990.00", "2.3"]],
                "a": [["45010.00", "1.2"], ["45020.00", "3.4"]]
            })";
            
            update.data = std::vector<uint8_t>(json.begin(), json.end());
            
            // Process the update
            normalizer->processMarketData(update);
            
            // Log stats periodically
            if (sequence_number % 100 == 0) {
                auto stats = publisher->getStats();
                std::cout << "Messages published: " << stats.messages_published 
                          << ", dropped: " << stats.messages_dropped 
                          << ", queue size: " << stats.current_queue_size << std::endl;
            }
        }
        
        // Clean shutdown
        std::cout << "Stopping publisher..." << std::endl;
        publisher->stop();
        
        std::cout << "Order Book Normalizer Service stopped" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}