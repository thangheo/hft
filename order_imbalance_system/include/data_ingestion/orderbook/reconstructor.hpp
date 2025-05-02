/**
 * @file reconstructor.hpp
 * @brief Core order book reconstruction logic
 * 
 * @details 
 * Part of the Data Ingestion & Processing Layer for the Crypto Trading System.
 * 
 * Created on: April 30, 2025
 */
#ifndef DATA_INGESTION_ORDERBOOK_RECONSTRUCTOR_HPP
#define DATA_INGESTION_ORDERBOOK_RECONSTRUCTOR_HPP

#include <memory>
#include <map>
#include <vector>
#include <mutex>
#include <string>

#include "data_ingestion/types.hpp"
#include "data_ingestion/orderbook/order_book.hpp"
#include "data_ingestion/publisher/publisher.hpp"

namespace data_ingestion {
namespace orderbook {

// Class for reconstructing and managing order books
class OrderBookReconstructor {
public:
    static OrderBookReconstructor& getInstance();
    
    // Initialization
    bool initialize(std::shared_ptr<publisher::Publisher> publisher);
    void shutdown();
    bool isInitialized() const { return initialized_; }
    bool isShutdown() const { return !initialized_; }
    void setPublisher(std::shared_ptr<publisher::Publisher> publisher) { publisher_ = publisher; }
    std::shared_ptr<publisher::Publisher> getPublisher() const { return publisher_; }
    std::string getPublisherAddress() const {
        return publisher_ ? publisher_->getAddress() : "";
    }
    
    // Order book updates
    bool processSnapshot(ExchangeId exchange, 
                        const Symbol& symbol,
                        const std::vector<PriceLevel>& bids,
                        const std::vector<PriceLevel>& asks,
                        uint64_t sequence_number,
                        Timestamp timestamp);
    
    bool processUpdate(ExchangeId exchange,
                      const Symbol& symbol,
                      const std::vector<PriceLevel>& bids,
                      const std::vector<PriceLevel>& asks,
                      uint64_t sequence_number,
                      Timestamp timestamp);
    
    // Order book access
    std::shared_ptr<OrderBook> getOrderBook(ExchangeId exchange, const Symbol& symbol) const;
    std::vector<std::shared_ptr<OrderBook>> getAllOrderBooks() const;
    std::vector<std::shared_ptr<OrderBook>> getOrderBooks(ExchangeId exchange) const;
    
    // State management
    bool hasOrderBook(ExchangeId exchange, const Symbol& symbol) const;
    void removeOrderBook(ExchangeId exchange, const Symbol& symbol);
    void clearOrderBooks();
    
    // Order book validation/recovery
    bool validateOrderBook(ExchangeId exchange, const Symbol& symbol);
    bool requestSnapshot(ExchangeId exchange, const Symbol& symbol);
    
private:
    OrderBookReconstructor();
    ~OrderBookReconstructor();
    OrderBookReconstructor(const OrderBookReconstructor&) = delete;
    OrderBookReconstructor& operator=(const OrderBookReconstructor&) = delete;
    
    // Key type for looking up order books
    struct OrderBookKey {
        ExchangeId exchange;
        Symbol symbol;
        
        bool operator<(const OrderBookKey& other) const {
            if (exchange != other.exchange) return exchange < other.exchange;
            return symbol < other.symbol;
        }
    };
    
    // Order book storage
    std::map<OrderBookKey, std::shared_ptr<OrderBook>> order_books_;
    
    // Publishing
    std::shared_ptr<publisher::Publisher> publisher_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Publish order book updates
    void publishOrderBook(const std::shared_ptr<OrderBook>& order_book);
};

} // namespace orderbook
} // namespace data_ingestion

#endif // DATA_INGESTION_ORDERBOOK_RECONSTRUCTOR_HPP