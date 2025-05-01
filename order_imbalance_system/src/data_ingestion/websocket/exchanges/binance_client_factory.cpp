// src/data_ingestion/websocket/exchanges/binance_client_factory.cpp
#include "data_ingestion/websocket/exchanges/binance_client_factory.hpp"

namespace trading {
namespace data_ingestion {

std::shared_ptr<BinanceWebSocketClient> BinanceClientFactory::createClient(
    MarketDataCallback callback,
    bool use_testnet) {
    
    const std::string& host = use_testnet ? TESTNET_HOST : MAINNET_HOST;
    
    return std::make_shared<BinanceWebSocketClient>(host, PORT, std::move(callback));
}

std::unique_ptr<ConnectionPool> BinanceClientFactory::createConnectionPool(
    MarketDataCallback callback,
    int min_connections,
    int max_connections,
    bool use_testnet) {
    
    const std::string& host = use_testnet ? TESTNET_HOST : MAINNET_HOST;
    
    // Create a factory function that will be used by the connection pool
    ConnectionFactory factory = [host, PORT, callback]() {
        return std::make_shared<BinanceWebSocketClient>(host, PORT, callback);
    };
    
    // Create the connection pool
    auto pool = std::make_unique<ConnectionPool>(
        "binance" + std::string(use_testnet ? "-testnet" : ""),
        factory,
        min_connections,
        max_connections
    );
    
    // Initialize the pool
    pool->initialize();
    
    return pool;
}

} // namespace data_ingestion
} // namespace trading