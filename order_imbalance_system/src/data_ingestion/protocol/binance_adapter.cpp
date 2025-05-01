/**
 * @file binance_adapter.cpp
 * @brief Implementation of Binance-specific message format conversion
 * 
 * @details 
 * Part of the Data Ingestion & Processing Layer for the Crypto Trading System.
 * 
 * Created on: April 30, 2025
 */

// TODO: Implement Implementation of Binance-specific message format conversion
// binance_adapter.cpp
#include "data_ingestion/protocol/binance_adapter.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool BinanceAdapter::canHandle(const ExchangeData& data) {
    // Check if this is Binance data
    try {
        const auto& jsonData = json::parse(data.data);
        return jsonData.contains("e") && 
               (jsonData["e"] == "depthUpdate" || jsonData["e"] == "depth");
    } catch (...) {
        return false;
    }
}

OrderBookUpdate BinanceAdapter::convertToStandardFormat(const ExchangeData& data) {
    OrderBookUpdate update;
    const auto& jsonData = json::parse(data.data);
    
    update.exchangeId = "binance";
    update.symbol = jsonData["s"];
    update.exchangeTime = jsonData["E"];
    update.receivedTime = data.receivedTime;
    update.sequenceNumber = jsonData["u"];
    update.isSnapshot = jsonData.contains("lastUpdateId");
    
    // Parse bids and asks
    for (const auto& bid : jsonData["b"]) {
        PriceLevel level;
        level.price = std::stod(bid[0].get<std::string>());
        level.quantity = std::stod(bid[1].get<std::string>());
        update.bids.push_back(level);
    }
    
    for (const auto& ask : jsonData["a"]) {
        PriceLevel level;
        level.price = std::stod(ask[0].get<std::string>());
        level.quantity = std::stod(ask[1].get<std::string>());
        update.asks.push_back(level);
    }
    
    // Some exchanges provide checksums
    if (jsonData.contains("checksum")) {
        update.checksum = jsonData["checksum"];
    }
    
    return update;
}