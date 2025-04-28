// src/data_ingestion/exchange_manager.cpp
#include "data_ingestion/exchange_manager.hpp"
#include <iostream>
#include <chrono>

namespace trading {
namespace data_ingestion {

ExchangeManager::ExchangeManager(NormalizedOrderBookCallback callback)
    : normalizer_(std::make_unique<OrderBookNormalizer>(std::move(callback))),
      running_(false) {
    
    // Start the connection monitor thread
    running_ = true;
    monitor_thread_ = std::thread(&ExchangeManager::monitorConnections, this);
}

ExchangeManager::~ExchangeManager() {
    // Stop the monitor thread
    running_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    // Disconnect from all exchanges
    disconnectAll();
}

bool ExchangeManager::addExchange(const std::string& exchange_id, 
                                 std::unique_ptr<ExchangeConnector> connector) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the exchange already exists
    if (connectors_.find(exchange_id) != connectors_.end()) {
        std::cerr << "Exchange " << exchange_id << " already exists" << std::endl;
        return false;
    }
    
    // Add the connector
    connectors_[exchange_id] = std::move(connector);
    std::cout << "Added exchange connector for " << exchange_id << std::endl;
    return true;
}

bool ExchangeManager::removeExchange(const std::string& exchange_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the exchange exists
    auto it = connectors_.find(exchange_id);
    if (it == connectors_.end()) {
        std::cerr << "Exchange " << exchange_id << " not found" << std::endl;
        return false;
    }
    
    // Disconnect and remove the connector
    it->second->disconnect();
    connectors_.erase(it);
    std::cout << "Removed exchange connector for " << exchange_id << std::endl;
    return true;
}

void ExchangeManager::connectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : connectors_) {
        const std::string& exchange_id = pair.first;
        ExchangeConnector* connector = pair.second.get();
        
        std::cout << "Connecting to " << exchange_id << "..." << std::endl;
        bool success = connector->connect();
        
        if (success) {
            std::cout << "Connected to " << exchange_id << std::endl;
        } else {
            std::cerr << "Failed to connect to " << exchange_id << ": " 
                      << connector->getLastError() << std::endl;
        }
    }
}

void ExchangeManager::disconnectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : connectors_) {
        const std::string& exchange_id = pair.first;
        ExchangeConnector* connector = pair.second.get();
        
        std::cout << "Disconnecting from " << exchange_id << "..." << std::endl;
        connector->disconnect();
    }
}

bool ExchangeManager::subscribe(const std::string& exchange_id, 
                               const std::vector<std::string>& trading_pairs,
                               const std::vector<DataType>& data_types,
                               int32_t depth) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the exchange exists
    auto it = connectors_.find(exchange_id);
    if (it == connectors_.end()) {
        std::cerr << "Exchange " << exchange_id << " not found" << std::endl;
        return false;
    }
    
    // Create a subscription request
    SubscriptionRequest request;
    request.exchange_id = exchange_id;
    request.trading_pairs = trading_pairs;
    request.data_types = data_types;
    request.depth = depth;
    
    // Subscribe
    ExchangeConnector* connector = it->second.get();
    bool success = connector->subscribe(request);
    
    if (success) {
        std::cout << "Subscribed to " << trading_pairs.size() << " trading pairs on " 
                  << exchange_id << std::endl;
    } else {
        std::cerr << "Failed to subscribe on " << exchange_id << ": " 
                  << connector->getLastError() << std::endl;
    }
    
    return success;
}

void ExchangeManager::subscribeAll(const std::vector<std::string>& trading_pairs,
                                  const std::vector<DataType>& data_types,
                                  int32_t depth) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : connectors_) {
        const std::string& exchange_id = pair.first;
        ExchangeConnector* connector = pair.second.get();
        
        // Create a subscription request
        SubscriptionRequest request;
        request.exchange_id = exchange_id;
        request.trading_pairs = trading_pairs;
        request.data_types = data_types;
        request.depth = depth;
        
        // Subscribe
        bool success = connector->subscribe(request);
        
        if (success) {
            std::cout << "Subscribed to " << trading_pairs.size() << " trading pairs on " 
                      << exchange_id << std::endl;
        } else {
            std::cerr << "Failed to subscribe on " << exchange_id << ": " 
                      << connector->getLastError() << std::endl;
        }
    }
}

std::vector<std::string> ExchangeManager::getConnectedExchanges() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    for (const auto& pair : connectors_) {
        if (pair.second->isConnected()) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

OrderBookUpdate ExchangeManager::getOrderBook(const std::string& exchange_id, 
                                            const std::string& trading_pair,
                                            int32_t depth) const {
    return normalizer_->getOrderBook(exchange_id, trading_pair, depth);
}

void ExchangeManager::onMarketData(const MarketDataUpdate& update) {
    // Process the market data through the normalizer
    normalizer_->processMarketData(update);
}

void ExchangeManager::monitorConnections() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            for (auto& pair : connectors_) {
                const std::string& exchange_id = pair.first;
                ExchangeConnector* connector = pair.second.get();
                
                if (!connector->isConnected()) {
                    std::cerr << "Exchange " << exchange_id << " disconnected, attempting reconnect..." << std::endl;
                    connector->connect();
                }
            }
        }
        
        // Check connections every 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

} // namespace data_ingestion
} // namespace trading