#pragma once

#include "../models/order.hpp"
#include "timer.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>

namespace trading {
namespace execution {
namespace algorithms {

/**
 * Configuration parameters for TWAP algorithm
 */
struct TwapConfig {
    double total_quantity;               // Total quantity to execute
    std::chrono::seconds duration;       // Duration of execution
    std::chrono::seconds slice_interval; // Time between order slices
    double price_limit = 0.0;            // Optional price limit, 0 for market orders
    double deviation_pct = 0.0;          // Max allowed deviation from target price
    bool allow_partial = true;           // Whether to allow partial fills
};

/**
 * Time-Weighted Average Price (TWAP) execution algorithm
 * Splits a large order into smaller equal-sized orders spread over time
 */
class TwapAlgorithm {
public:
    using OrderCallback = std::function<void(const models::OrderPtr&)>;
    using CompletionCallback = std::function<void(bool, const std::string&)>;
    
    // Constructor
    TwapAlgorithm();
    
    // Initialize the algorithm with configuration
    bool initialize(
        const std::string& trading_pair,
        models::OrderSide side,
        const TwapConfig& config);
    
    // Start algorithm execution
    bool start();
    
    // Pause execution
    bool pause();
    
    // Resume execution
    bool resume();
    
    // Cancel execution
    bool cancel();
    
    // Register callbacks
    void setOrderCallback(const OrderCallback& callback);
    void setCompletionCallback(const CompletionCallback& callback);
    
    // Handle order updates
    void handleOrderUpdate(const models::OrderPtr& order);
    
    // Get algorithm status
    bool isRunning() const;
    bool isComplete() const;
    bool isCancelled() const;
    
    // Get execution statistics
    double getTotalQuantity() const;
    double getExecutedQuantity() const;
    double getRemainingQuantity() const;
    double getAverageExecutionPrice() const;
    std::chrono::seconds getRemainingTime() const;
    
    // Get all child orders
    std::vector<models::OrderPtr> getChildOrders() const;
    
private:
    // Algorithm state
    enum class State {
        INITIALIZED,
        RUNNING,
        PAUSED,
        COMPLETED,
        CANCELLED,
        ERROR
    };
    
    // Internal implementation
    std::string trading_pair_;
    models::OrderSide side_;
    TwapConfig config_;
    State state_ = State::INITIALIZED;
    std::string error_message_;
    
    // Execution tracking
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point end_time_;
    std::vector<models::OrderPtr> child_orders_;
    double executed_quantity_ = 0.0;
    double average_execution_price_ = 0.0;
    
    // Timer for slicing
    std::unique_ptr<class Timer> timer_;
    
    // Callbacks
    OrderCallback order_callback_;
    CompletionCallback completion_callback_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
    
    // Internal methods
    void scheduleNextSlice();
    void createAndSendSlice();
    void updateExecutionStatistics();
    void checkCompletion();
    void setErrorState(const std::string& error_message);
};

} // namespace algorithms
} // namespace execution
} // namespace trading