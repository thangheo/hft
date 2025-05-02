// tests/data_ingestion/test_order_book.cpp
#include "data_ingestion/orderbook/order_book.hpp"
#include "data_ingestion/orderbook/order_book_types.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace trading::data;

// Helper function to create price levels
std::vector<PriceLevel> createPriceLevels(const std::vector<std::pair<std::string, std::string>>& prices_quantities) {
    std::vector<PriceLevel> levels;
    for (const auto& [price, quantity] : prices_quantities) {
        PriceLevel level;
        level.price = price;
        level.quantity = quantity;
        levels.push_back(level);
    }
    return levels;
}

// Test basic order book functionality
void testOrderBookBasic() {
    OrderBookInstance order_book("binance", "btcusdt");
    
    // Create initial snapshot
    auto bids = createPriceLevels({
        {"10100.00", "1.5"},
        {"10099.00", "2.3"},
        {"10098.00", "3.1"}
    });
    
    auto asks = createPriceLevels({
        {"10101.00", "1.2"},
        {"10102.00", "2.4"},
        {"10103.00", "3.0"}
    });
    
    // Process snapshot
    bool result = order_book.processSnapshot(1, 1000, bids, asks);
    assert(result == true);
    assert(order_book.isValid() == true);
    
    // Get order book
    std::vector<PriceLevel> bids_result, asks_result;
    result = order_book.getOrderBook(10, bids_result, asks_result);
    
    assert(result == true);
    assert(bids_result.size() == 3);
    assert(asks_result.size() == 3);
    
    // Check values
    assert(bids_result[0].price == "10100.00");
    assert(bids_result[0].quantity == "1.5");
    assert(asks_result[0].price == "10101.00");
    assert(asks_result[0].quantity == "1.2");
    
    std::cout << "Basic order book test passed" << std::endl;
}

// Test incremental updates
void testOrderBookUpdates() {
    OrderBookInstance order_book("binance", "btcusdt");
    
    // Create initial snapshot
    auto bids = createPriceLevels({
        {"10100.00", "1.5"},
        {"10099.00", "2.3"},
        {"10098.00", "3.1"}
    });
    
    auto asks = createPriceLevels({
        {"10101.00", "1.2"},
        {"10102.00", "2.4"},
        {"10103.00", "3.0"}
    });
    
    // Process snapshot
    bool result = order_book.processSnapshot(1, 1000, bids, asks);
    assert(result == true);
    
    // Process an update - add new level and update existing
    auto bids_update = createPriceLevels({
        {"10100.00", "2.0"},  // Update existing level
        {"10101.00", "0.5"}   // Add new level at top
    });
    
    auto asks_update = createPriceLevels({
        {"10101.00", "0.5"},  // Update existing level
        {"10100.50", "1.0"}   // Add new level at top
    });
    
    std::vector<std::string> bids_remove = {"10098.00"};  // Remove lowest bid
    std::vector<std::string> asks_remove = {"10103.00"};  // Remove highest ask
    
    // Process update
    result = order_book.processUpdate(2, 2000, bids_update, asks_update, bids_remove, asks_remove);
    assert(result == true);
    
    // Get updated order book
    std::vector<PriceLevel> bids_result, asks_result;
    result = order_book.getOrderBook(10, bids_result, asks_result);
    
    // Should have 3 levels for each side
    assert(bids_result.size() == 3);
    assert(asks_result.size() == 3);
    
    // Check updated values
    assert(bids_result[0].price == "10101.00");  // New top bid
    assert(bids_result[0].quantity == "0.5");
    assert(bids_result[1].price == "10100.00");  // Updated bid
    assert(bids_result[1].quantity == "2.0");
    assert(bids_result[2].price == "10099.00");  // Unchanged bid
    
    assert(asks_result[0].price == "10100.50");  // New top ask
    assert(asks_result[0].quantity == "1.0");
    assert(asks_result[1].price == "10101.00");  // Updated ask
    assert(asks_result[1].quantity == "0.5");
    assert(asks_result[2].price == "10102.00");  // Unchanged ask
    
    std::cout << "Order book updates test passed" << std::endl;
}

// Test order book metrics
void testOrderBookMetrics() {
    OrderBookInstance order_book("binance", "btcusdt");
    
    // Create initial snapshot
    auto bids = createPriceLevels({
        {"10100.00", "1.5"},
        {"10099.00", "2.3"},
        {"10098.00", "3.1"}
    });
    
    auto asks = createPriceLevels({
        {"10101.00", "1.2"},
        {"10102.00", "2.4"},
        {"10103.00", "3.0"}
    });
    
    // Process snapshot
    bool result = order_book.processSnapshot(1, 1000, bids, asks);
    assert(result == true);
    
    // Get metrics
    std::unordered_map<std::string, double> metrics;
    result = order_book.getMetrics(metrics);
    assert(result == true);
    
    // Check metrics
    assert(metrics.count("bid_volume") > 0);
    assert(metrics.count("ask_volume") > 0);
    assert(metrics.count("bid_ask_ratio") > 0);
    assert(metrics.count("mid_price") > 0);
    
    double bid_volume = metrics["bid_volume"];
    double ask_volume = metrics["ask_volume"];
    double bid_ask_ratio = metrics["bid_ask_ratio"];
    double mid_price = metrics["mid_price"];
    
    // Expected values
    double expected_bid_volume = 1.5 + 2.3 + 3.1;
    double expected_ask_volume = 1.2 + 2.4 + 3.0;
    double expected_ratio = expected_bid_volume / expected_ask_volume;
    double expected_mid_price = (10100.0 + 10101.0) / 2.0;
    
    // Check with some tolerance for floating point comparison
    assert(std::abs(bid_volume - expected_bid_volume) < 0.001);
    assert(std::abs(ask_volume - expected_ask_volume) < 0.001);
    assert(std::abs(bid_ask_ratio - expected_ratio) < 0.001);
    assert(std::abs(mid_price - expected_mid_price) < 0.001);
    
    std::cout << "Order book metrics test passed" << std::endl;
}

// Test manager functionality
void testOrderBookManager() {
    OrderBookManager manager;
    
    // Create order books
    auto book1 = manager.getOrCreateOrderBook("binance", "btcusdt");
    auto book2 = manager.getOrCreateOrderBook("binance", "ethusdt");
    
    // Verify books were created
    assert(book1 != nullptr);
    assert(book2 != nullptr);
    
    // Try to get existing book
    auto book1_again = manager.getOrCreateOrderBook("binance", "btcusdt");
    assert(book1 == book1_again);  // Should be the same instance
    
    // Get all books
    auto all_books = manager.getAllOrderBooks();
    assert(all_books.size() == 2);
    
    // Remove a book
    bool result = manager.removeOrderBook("binance", "btcusdt");
    assert(result == true);
    
    // Book should be gone
    all_books = manager.getAllOrderBooks();
    assert(all_books.size() == 1);
    
    std::cout << "Order book manager test passed" << std::endl;
}

int main() {
    // Run all tests
    testOrderBookBasic();
    testOrderBookUpdates();
    testOrderBookMetrics();
    testOrderBookManager();
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
