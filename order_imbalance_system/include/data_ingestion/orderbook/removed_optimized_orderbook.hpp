// optimized_orderbook.h
#include <atomic>
#include <tbb/concurrent_unordered_map.h>
#include <folly/concurrency/ConcurrentHashMap.h>
#include <boost/lockfree/spsc_queue.hpp>

// Specialized price level container with cache-friendly layout
class OptimizedPriceLevels {
public:
    // Constructor with pre-allocation for better performance
    OptimizedPriceLevels(size_t initialCapacity = 100) {
        // Pre-allocate memory to avoid reallocations
        prices.reserve(initialCapacity);
        quantities.reserve(initialCapacity);
        indices.reserve(initialCapacity);
    }
    
    void update(double price, double quantity) {
        auto it = indices.find(price);
        if (it != indices.end()) {
            size_t idx = it->second;
            if (quantity > 0) {
                // Update existing level
                quantities[idx] = quantity;
            } else {
                // Remove level
                removeLevel(idx);
                indices.erase(it);
            }
        } else if (quantity > 0) {
            // Add new level
            size_t idx = prices.size();
            prices.push_back(price);
            quantities.push_back(quantity);
            indices[price] = idx;
        }
    }
    
    void clear() {
        prices.clear();
        quantities.clear();
        indices.clear();
    }
    
    // Get levels sorted by price (ascending or descending)
    std::vector<PriceLevel> getLevels(bool ascending = true, size_t limit = 0) const {
        std::vector<size_t> sortedIndices(prices.size());
        std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
        
        // Sort indices by price
        std::sort(sortedIndices.begin(), sortedIndices.end(), 
            [this, ascending](size_t a, size_t b) {
                return ascending ? (prices[a] < prices[b]) : (prices[a] > prices[b]);
            });
        
        // Build result
        std::vector<PriceLevel> result;
        size_t count = 0;
        for (size_t idx : sortedIndices) {
            PriceLevel level;
            level.price = prices[idx];
            level.quantity = quantities[idx];
            result.push_back(level);
            
            count++;
            if (limit > 0 && count >= limit) break;
        }
        
        return result;
    }
    
    double getBestPrice(bool lowest = true) const {
        if (prices.empty()) return 0.0;
        
        if (lowest) {
            return *std::min_element(prices.begin(), prices.end());
        } else {
            return *std::max_element(prices.begin(), prices.end());
        }
    }
    
    size_t size() const {
        return prices.size();
    }
    
private:
    void removeLevel(size_t idx) {
        // Swap with last element and pop (to avoid shifting elements)
        size_t lastIdx = prices.size() - 1;
        if (idx != lastIdx) {
            // Update the index map for the element being moved
            indices[prices[lastIdx]] = idx;
            
            // Move last element to the removed position
            prices[idx] = prices[lastIdx];
            quantities[idx] = quantities[lastIdx];
        }
        
        // Remove last element
        prices.pop_back();
        quantities.pop_back();
    }

    // Store price levels in separate vectors for better cache locality
    std::vector<double> prices;
    std::vector<double> quantities;
    std::unordered_map<double, size_t> indices;  // Maps price to index in the vectors
};

// High-performance order book implementation
class HighPerformanceOrderBook {
public:
    HighPerformanceOrderBook() {
        // Set initial sequence number
        sequenceNumber.store(0);
    }
    
    void applyUpdate(const OrderBookUpdate& update) {
        // Check if this is a newer update
        uint64_t currentSeq = sequenceNumber.load(std::memory_order_relaxed);
        if (update.sequenceNumber <= currentSeq && currentSeq > 0) {
            // Skip older updates
            return;
        }
        
        // Process the update
        if (update.isSnapshot) {
            // Replace entire book
            bids.clear();
            asks.clear();
            
            for (const auto& bid : update.bids) {
                bids.update(bid.price, bid.quantity);
            }
            
            for (const auto& ask : update.asks) {
                asks.update(ask.price, ask.quantity);
            }
        } else {
            // Incremental update
            for (const auto& bid : update.bids) {
                bids.update(bid.price, bid.quantity);
            }
            
            for (const auto& ask : update.asks) {
                asks.update(ask.price, ask.quantity);
            }
        }
        
        // Update sequence number
        sequenceNumber.store(update.sequenceNumber, std::memory_order_release);
        lastUpdateTime.store(update.processedTime, std::memory_order_relaxed);
    }
    
    OrderBookSnapshot getSnapshot(size_t depth = 0) const {
        OrderBookSnapshot snapshot;
        snapshot.exchangeId = exchangeId;
        snapshot.symbol = symbol;
        snapshot.timestamp = lastUpdateTime.load(std::memory_order_relaxed);
        
        // Get sorted price levels
        snapshot.bids = bids.getLevels(false, depth);  // Descending
        snapshot.asks = asks.getLevels(true, depth);   // Ascending
        
        // Calculate derived metrics
        double bestBid = bids.getBestPrice(false);  // Highest bid
        double bestAsk = asks.getBestPrice(true);   // Lowest ask
        
        if (bestBid > 0 && bestAsk > 0) {
            snapshot.spread = bestAsk - bestBid;
            snapshot.midPrice = (bestBid + bestAsk) / 2.0;
        } else {
            snapshot.spread = -1.0;
            snapshot.midPrice = -1.0;
        }
        
        return snapshot;
    }
    
    void setInfo(const std::string& exId, const std::string& sym) {
        exchangeId = exId;
        symbol = sym;
    }
    
    uint64_t getSequenceNumber() const {
        return sequenceNumber.load(std::memory_order_acquire);
    }
    
private:
    OptimizedPriceLevels bids;
    OptimizedPriceLevels asks;
    std::atomic<uint64_t> sequenceNumber;
    std::atomic<Timestamp> lastUpdateTime;
    std::string exchangeId;
    std::string symbol;
};

// Thread-safe access to multiple order books
class OrderBookManager {
public:
    OrderBookManager() {}
    
    void processUpdate(const OrderBookUpdate& update) {
        std::string key = getKey(update.exchangeId, update.symbol);
        
        // Get or create order book
        auto& book = orderBooks[key];
        if (!book) {
            book = std::make_shared<HighPerformanceOrderBook>();
            book->setInfo(update.exchangeId, update.symbol);
        }
        
        // Apply update
        book->applyUpdate(update);
        
        // Notify subscribers if needed
        notifySubscribers(update, key);
    }
    
    OrderBookSnapshot getSnapshot(const std::string& exchangeId, 
                                 const std::string& symbol,
                                 size_t depth = 0) {
        std::string key = getKey(exchangeId, symbol);
        
        auto it = orderBooks.find(key);
        if (it != orderBooks.end() && it->second) {
            return it->second->getSnapshot(depth);
        }
        
        // Return empty snapshot if book doesn't exist
        OrderBookSnapshot emptySnapshot;
        emptySnapshot.exchangeId = exchangeId;
        emptySnapshot.symbol = symbol;
        emptySnapshot.timestamp = getCurrentTimestamp();
        return emptySnapshot;
    }
    
    void registerCallback(const std::function<void(const OrderBookUpdate&, const std::string&)>& callback) {
        callbacks.push_back(callback);
    }
    
private:
    std::string getKey(const std::string& exchangeId, const std::string& symbol) const {
        return exchangeId + ":" + symbol;
    }
    
    void notifySubscribers(const OrderBookUpdate& update, const std::string& key) {
        for (const auto& callback : callbacks) {
            callback(update, key);
        }
    }
    
    Timestamp getCurrentTimestamp() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    // Use concurrent hash map for thread safety
    folly::ConcurrentHashMap<std::string, std::shared_ptr<HighPerformanceOrderBook>> orderBooks;
    std::vector<std::function<void(const OrderBookUpdate&, const std::string&)>> callbacks;
};