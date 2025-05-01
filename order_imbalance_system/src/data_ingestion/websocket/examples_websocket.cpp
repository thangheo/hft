// src/data_ingestion/examples/websocket_example.cpp
#include "data_ingestion/websocket/exchanges/binance_client_factory.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <csignal>
#include <atomic>

using namespace trading::data_ingestion;

// Signal handler to gracefully exit the program
std::atomic<bool> running{true};

void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Market data callback function
void onMarketData(const MarketDataUpdate& update) {
    std::cout << "Received " << update.update_type << " update from " 
              << update.exchange_id << " for " << update.trading_pair << std::endl;
    
    // Print the first few bytes of the data for debugging
    size_t print_size = std::min(size_t(50), update.data.size());
    std::string data_preview(update.data.begin(), update.data.begin() + print_size);
    if (update.data.size() > print_size) {
        data_preview += "...";
    }
    
    std::cout << "Data preview: " << data_preview << std::endl;
    
    // Calculate latency
    auto received_ns = std::chrono::system_clock::now().time_since_epoch().count();
    auto latency_ms = (received_ns - update.received_timestamp) / 1000000.0;
    std::cout << "Processing latency: " << latency_ms << "ms" << std::endl;
    
    std::cout << "-------------------------" << std::endl;
}

int main() {
    // Set up signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    try {
        std::cout << "Starting WebSocket example..." << std::endl;
        
        // Create a connection pool for Binance
        auto pool = BinanceClientFactory::createConnectionPool(
            onMarketData,   // Callback function
            2,              // Min connections
            5,              // Max connections
            false           // Use production API
        );
        
        std::cout << "Connection pool created and initialized" << std::endl;
        
        // Acquire a connection from the pool
        auto client = std::dynamic_pointer_cast<BinanceWebSocketClient>(pool->acquireConnection());
        
        if (!client) {
            std::cerr << "Failed to acquire a connection from the pool" << std::endl;
            return 1;
        }
        
        std::cout << "Acquired connection from pool" << std::endl;
        
        // Subscribe to some streams
        std::vector<std::string> streams = {
            "btcusdt@depth@100ms",
            "btcusdt@trade",
            "ethusdt@depth@100ms",
            "ethusdt@trade"
        };
        
        if (client->subscribe(streams)) {
            std::cout << "Successfully subscribed to " << streams.size() << " streams" << std::endl;
        } else {
            std::cerr << "Failed to subscribe to streams" << std::endl;
            return 1;
        }
        
        // Keep the program running until a signal is received
        std::cout << "Receiving data... Press Ctrl+C to exit" << std::endl;
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            if (pool->isHealthy()) {
                std::cout << "Connection pool status: Healthy ("
                          << pool->getActiveConnectionCount() << " active, "
                          << pool->getAvailableConnectionCount() << " available, "
                          << pool->getTotalConnectionCount() << " total)" << std::endl;
            } else {
                std::cerr << "Connection pool status: Unhealthy" << std::endl;
            }
        }
        
        // Release the connection back to the pool
        pool->releaseConnection(client);
        
        // Shutdown the pool
        std::cout << "Shutting down connection pool..." << std::endl;
        pool->shutdown();
        
        std::cout << "Example completed successfully" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}