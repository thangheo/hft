// order_book.cpp
#include "order_book.h"

void OrderBook::applyUpdate(const OrderBookUpdate& update) {
    // Lock for thread safety during update
    std::unique_lock<std::shared_mutex> lock(bookMutex);
    
    // Handle snapshot
    if (update.isSnapshot) {
        bids.clear();
        asks.clear();
    }
    
    // Update bids
    for (const auto& bidLevel : update.bids) {
        double price = bidLevel.price;
        double quantity = bidLevel.quantity;
        
        if (quantity > 0) {
            // Add or update price level
            bids[price] = bidLevel;
        } else {
            // Remove price level
            bids.erase(price);
        }
    }
    
    // Update asks
    for (const auto& askLevel : update.asks) {
        double price = askLevel.price;
        double quantity = askLevel.quantity;
        
        if (quantity > 0) {
            // Add or update price level
            asks[price] = askLevel;
        } else {
            // Remove price level
            asks.erase(price);
        }
    }
    
    // Update metadata
    lastUpdateTime = update.processedTime;
    sequenceNumber = update.sequenceNumber;
}

std::vector<PriceLevel> OrderBook::getBids(size_t depth) const {
    std::shared_lock<std::shared_mutex> lock(bookMutex);
    
    std::vector<PriceLevel> result;
    size_t count = 0;
    
    for (const auto& [price, level] : bids) {
        result.push_back(level);
        count++;
        if (depth > 0 && count >= depth) break;
    }
    
    return result;
}

std::vector<PriceLevel> OrderBook::getAsks(size_t depth) const {
    std::shared_lock<std::shared_mutex> lock(bookMutex);
    
    std::vector<PriceLevel> result;
    size_t count = 0;
    
    for (const auto& [price, level] : asks) {
        result.push_back(level);
        count++;
        if (depth > 0 && count >= depth) break;
    }
    
    return result;
}

double OrderBook::getBidAskSpread() const {
    std::shared_lock<std::shared_mutex> lock(bookMutex);
    
    if (bids.empty() || asks.empty()) return -1.0;
    
    double bestBid = bids.begin()->first;
    double bestAsk = asks.begin()->first;
    
    return bestAsk - bestBid;
}

double OrderBook::getMidPrice() const {
    std::shared_lock<std::shared_mutex> lock(bookMutex);
    
    if (bids.empty() || asks.empty()) return -1.0;
    
    double bestBid = bids.begin()->first;
    double bestAsk = asks.begin()->first;
    
    return (bestBid + bestAsk) / 2.0;
}

bool OrderBook::validateChecksum(const std::string& checksum) const {
    // Implement exchange-specific checksum validation
    // This can vary by exchange
    return calculateChecksum() == checksum;
}

std::string OrderBook::calculateChecksum() const {
    // Example implementation for Binance checksums
    // Each exchange may have a different algorithm
    std::stringstream ss;
    
    // Take first N levels from each side
    int n = 10;
    int count = 0;
    
    for (const auto& [price, level] : bids) {
        if (count >= n) break;
        ss << std::fixed << std::setprecision(8) << price << ":" 
           << std::fixed << std::setprecision(8) << level.quantity << ":";
        count++;
    }
    
    count = 0;
    for (const auto& [price, level] : asks) {
        if (count >= n) break;
        ss << std::fixed << std::setprecision(8) << price << ":" 
           << std::fixed << std::setprecision(8) << level.quantity << ":";
        count++;
    }
    
    std::string data = ss.str();
    if (!data.empty()) {
        data.pop_back(); // Remove trailing colon
    }
    
    // CRC32 checksum algorithm
    return calculateCRC32(data);
}