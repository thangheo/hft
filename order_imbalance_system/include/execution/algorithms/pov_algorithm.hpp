#pragma once

#include "execution/models/order.hpp"
#include "execution/algorithms/pov_config.hpp"
#include "execution/algorithms/timer.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <algorithm>

namespace trading {
namespace execution {

// Forward declarations
struct ExecutionResponse;
class ExecutionService;

namespace algorithms {
// struct ExecutionResponse;

/**
 * Percentage of Volume (POV) execution algorithm
 * Trades at a specified percentage of market volume to minimize market impact
 */
class PovAlgorithm {
public:
    using OrderCallback = std::function<void(const models::OrderPtr&)>;
    using CompletionCallback = std::function<void(bool, const std::string&)>;
    using MarketVolumeCallback = std::function<double(const std::string&, std::chrono::minutes)>;
    
    // Constructor
    PovAlgorithm();
    
    // Initialize the algorithm with configuration
    bool initialize(
        const std::string& trading_pair,
        models::OrderSide side,
        const PovConfig& config);
    
    // Start algorithm execution
    bool start();
    
    // Pause execution
    bool pause();
    
    // Resume execution
    bool cancel();
    
    /**
     * @brief Execute a parent order using POV algorithm
     * 
     * @param parentOrder The parent order with POV parameters
     * @param execution_service Reference to the execution service for placing child orders
     * @return ExecutionResponse with initial execution status
     */
    ExecutionResponse execute(const models::Order& parentOrder, ExecutionService& execution_service);
    
    // Register callbacks
    void setOrderCallback(const OrderCallback& callback);
    void setCompletionCallback(const CompletionCallback& callback);
    void setMarketVolumeCallback(const MarketVolumeCallback& callback);
    
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
    double getParticipationRate() const;
    std::chrono::seconds getRemainingTime() const;
    
    // Get all child orders
    std::vector<models::OrderPtr> getChildOrders() const;
    
private:
    // Extract POV configuration from order parameters
    PovConfig extractConfigFromOrder(const models::Order& parentOrder);
    
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
    PovConfig config_;
    State state_ = State::INITIALIZED;
    std::string error_message_;
    
    // Execution tracking
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point end_time_;
    std::vector<models::OrderPtr> child_orders_;
    double executed_quantity_ = 0.0;
    double average_execution_price_ = 0.0;
    
    // Market volume tracking
    std::deque<double> recent_volumes_;
    std::chrono::system_clock::time_point last_volume_check_;
    double cumulative_market_volume_ = 0.0;
    double target_participation_rate_ = 0.0;
    
    // Timer for volume checks
    std::unique_ptr<Timer> timer_;
    
    // Service reference for order placement
    ExecutionService* execution_service_ = nullptr;
    
    // Callbacks
    OrderCallback order_callback_;
    CompletionCallback completion_callback_;
    MarketVolumeCallback market_volume_callback_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
    
    // Internal methods
    void checkMarketVolume();
    void calculateOrderSize();
    void placeChildOrder(double quantity);
    void updateExecutionStatistics();
    void checkCompletion();
    void setErrorState(const std::string& error_message);
    double getMarketVolumeForPeriod(std::chrono::minutes period);
    double calculateOptimalOrderSize();
};

/**
 * Configuration parameters for POV algorithm
 */
// struct PovConfig {
//     double total_quantity;               // Total quantity to execute
//     std::chrono::seconds duration;       // Maximum duration of execution
//     double participation_rate;           // Target percentage of market volume (0.01-1.0)
//     double min_order_size = 0.0;         // Minimum size for each child order
//     double max_order_size = 0.0;         // Maximum size for each child order
//     std::chrono::seconds check_interval = std::chrono::seconds(30); // Interval for checking market volume
//     double price_limit = 0.0;            // Optional price limit, 0 for market orders
//     double deviation_pct = 0.01;         // Max allowed deviation from target price (1%)
//     bool dynamic_rate = false;           // Whether to adjust participation rate dynamically
//     std::chrono::minutes volume_lookback = std::chrono::minutes(5); // Period to look back for volume analysis
//     std::string exchange_id;             // Target exchange for execution
//     double urgency_factor = 0.5;         // 0-1 factor for urgency (affects participation rate)
// };

} // namespace algorithms
} // namespace execution
} // namespace trading