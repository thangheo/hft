// include/data_ingestion/order_book.h
#pragma once

#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace trading {
namespace data {

// Forward declarations
class OrderBookInstance;

// Represents a price level in the order book
struct PriceLevel {
    std::string price;      // Using string to preserve precision
    std::string quantity;   // Using string to preserve precision
    std::unordered_map<std::string, std::string> orders; // order_id -> quantity
    
    // Constructor
    PriceLevel(const std::string& p = "", const std::string& q = "")
        : price(p), quantity(q) {}
};

// Represents a single order book for a trading pair on a specific exchange
class OrderBookInstance {
public:
    OrderBookInstance(const std::string& exchange_id, 
                     const std::string& trading_pair,
                     bool track_individual_orders = false);
    
    // Process an order book snapshot (full refresh)
    bool processSnapshot(int64_t sequence_number,
                        int64_t timestamp,
                        const std::vector<PriceLevel>& bids,
                        const std::vector<PriceLevel>& asks);
    
    // Process an incremental update
    bool processUpdate(int64_t sequence_number,
                      int64_t timestamp,
                      const std::vector<PriceLevel>& bids_to_add,
                      const std::vector<PriceLevel>& asks_to_add,
                      const std::vector<std::string>& bids_to_remove,
                      const std::vector<std::string>& asks_to_remove);
    
    // Get current order book state
    bool getOrderBook(int max_depth,
                     std::vector<PriceLevel>& bids,
                     std::vector<PriceLevel>& asks);
    
    // Check if order book is valid and up to date
    bool isValid() const;
    
    // Get order book metrics
    bool getMetrics(std::unordered_map<std::string, double>& metrics);
    
    // Getters
    std::string getExchangeId() const { return exchange_id_; }
    std::string getTradingPair() const { return trading_pair_; }
    int64_t getLastUpdateTime() const { return last_update_time_; }
    int64_t getSequenceNumber() const { return last_sequence_number_; }
    
private:
    std::string exchange_id_;
    std::string trading_pair_;
    bool track_individual_orders_;
    
    // Using map for sorted price levels (bids descending, asks ascending)
    std::map<std::string, PriceLevel, std::greater<std::string>> bids_;
    std::map<std::string, PriceLevel, std::less<std::string>> asks_;
    
    std::atomic<int64_t> last_update_time_;
    std::atomic<int64_t> last_sequence_number_;
    std::atomic<bool> is_valid_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
    
    // Calculate checksum for validation
    std::string calculateChecksum() const;
    
    // Internal helper methods
    void updateBidLevel(const PriceLevel& level);
    void updateAskLevel(const PriceLevel& level);
    void removeBidLevel(const std::string& price);
    void removeAskLevel(const std::string& price);
};

// Manager class to coordinate multiple order book instances
class OrderBookReconstructorManager {
public:
    OrderBookReconstructorManager();
    ~OrderBookReconstructorManager();
    
    // Get or create an order book instance
    std::shared_ptr<OrderBookInstance> getOrCreateOrderBook(
        const std::string& exchange_id,
        const std::string& trading_pair);
    
    // Remove an order book instance
    bool removeOrderBook(const std::string& exchange_id,
                        const std::string& trading_pair);
    
    // Get all active order books
    std::vector<std::shared_ptr<OrderBookInstance>> getAllOrderBooks() const;
    
    // Start publishing order book data
    bool startPublishing(const std::string& publish_endpoint);
    
    // Stop publishing
    void stopPublishing();
    
private:
    std::unordered_map<std::string, std::shared_ptr<OrderBookInstance>> order_books_;
    mutable std::mutex mutex_;
    
    // ZeroMQ publisher
    void* zmq_context_;
    void* zmq_publisher_;
    bool is_publishing_;
    std::thread publisher_thread_;
    
    // Thread for publishing order book updates
    void publisherThreadFunc();
    
    // Generate a unique key for an order book
    std::string generateKey(const std::string& exchange_id,
                          const std::string& trading_pair) const;
};

} // namespace data
} // namespace trading
