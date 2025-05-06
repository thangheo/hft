// algorithms/twap_algorithm.cpp
#include "../include/execution/algorithms/twap_algorithm.hpp"
#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>

namespace trading {
namespace execution {
namespace algorithms {

// Simple timer implementation for scheduling slices
class Timer {
public:
    Timer(std::function<void(void)> callback, std::chrono::milliseconds interval)
        : callback_(callback), interval_(interval), running_(false) {}
    
    ~Timer() {
        stop();
    }
    
    void start() {
        running_ = true;
        thread_ = std::thread([this]() {
            while (running_) {
                std::this_thread::sleep_for(interval_);
                if (running_) {
                    callback_();
                }
            }
        });
    }
    
    void stop() {
        running_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    
private:
    std::function<void(void)> callback_;
    std::chrono::milliseconds interval_;
    std::thread thread_;
    std::atomic<bool> running_;
};

TwapAlgorithm::TwapAlgorithm() = default;

bool TwapAlgorithm::initialize(
    const std::string& trading_pair,
    models::OrderSide side,
    const TwapConfig& config) {
    
    // Lock to prevent concurrent modification
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate configuration
    if (config.total_quantity <= 0) {
        setErrorState("Total quantity must be positive");
        return false;
    }
    
    if (config.duration.count() <= 0) {
        setErrorState("Duration must be positive");
        return false;
    }
    
    if (config.slice_interval.count() <= 0) {
        setErrorState("Slice interval must be positive");
        return false;
    }
    
    if (config.slice_interval > config.duration) {
        setErrorState("Slice interval cannot be greater than total duration");
        return false;
    }
    
    // Store parameters
    trading_pair_ = trading_pair;
    side_ = side;
    config_ = config;
    state_ = State::INITIALIZED;
    executed_quantity_ = 0.0;
    average_execution_price_ = 0.0;
    
    // Clear any existing child orders
    child_orders_.clear();
    
    return true;
}

bool TwapAlgorithm::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (state_ != State::INITIALIZED && state_ != State::PAUSED) {
        return false;
    }
    
    // Setup execution timing
    start_time_ = std::chrono::system_clock::now();
    end_time_ = start_time_ + config_.duration;
    
    // Start slicing timer
    if (!timer_) {
        timer_ = std::make_unique<Timer>(
            [this]() { createAndSendSlice(); },
            std::chrono::duration_cast<std::chrono::milliseconds>(config_.slice_interval)
        );
    }
    
    state_ = State::RUNNING;
    timer_->start();
    
    // Create and send first slice immediately
    createAndSendSlice();
    
    return true;
}

bool TwapAlgorithm::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (state_ != State::RUNNING) {
        return false;
    }
    
    if (timer_) {
        timer_->stop();
    }
    
    state_ = State::PAUSED;
    return true;
}

bool TwapAlgorithm::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (state_ != State::PAUSED) {
        return false;
    }
    
    if (timer_) {
        timer_->start();
    }
    
    state_ = State::RUNNING;
    return true;
}

bool TwapAlgorithm::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (state_ == State::COMPLETED || state_ == State::CANCELLED) {
        return false;
    }
    
    if (timer_) {
        timer_->stop();
    }
    
    // Cancel active orders
    for (const auto& order : child_orders_) {
        if (order->getState() == models::OrderState::OPEN ||
            order->getState() == models::OrderState::PARTIALLY_FILLED) {
            // In a real implementation, we would call the order cancellation API here
        }
    }
    
    state_ = State::CANCELLED;
    
    // Notify completion with cancelled status
    if (completion_callback_) {
        completion_callback_(false, "Algorithm cancelled by user");
    }
    
    return true;
}

void TwapAlgorithm::setOrderCallback(const OrderCallback& callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    order_callback_ = callback;
}

void TwapAlgorithm::setCompletionCallback(const CompletionCallback& callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    completion_callback_ = callback;
}

void TwapAlgorithm::handleOrderUpdate(const models::OrderPtr& order) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find the order in our child orders
    auto it = std::find_if(child_orders_.begin(), child_orders_.end(),
        [&order](const models::OrderPtr& child) {
            return child->getOrderId() == order->getOrderId();
        });
    
    if (it != child_orders_.end()) {
        // Update our copy with the latest state
        *it = order;
        
        // Update execution statistics
        updateExecutionStatistics();
        
        // Check if we need to create more slices or if we're done
        checkCompletion();
    }
}

bool TwapAlgorithm::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_ == State::RUNNING;
}

bool TwapAlgorithm::isComplete() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_ == State::COMPLETED;
}

bool TwapAlgorithm::isCancelled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_ == State::CANCELLED;
}

double TwapAlgorithm::getTotalQuantity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.total_quantity;
}

double TwapAlgorithm::getExecutedQuantity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return executed_quantity_;
}

double TwapAlgorithm::getRemainingQuantity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.total_quantity - executed_quantity_;
}

double TwapAlgorithm::getAverageExecutionPrice() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return average_execution_price_;
}

std::chrono::seconds TwapAlgorithm::getRemainingTime() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (state_ == State::COMPLETED || state_ == State::CANCELLED) {
        return std::chrono::seconds(0);
    }
    
    auto now = std::chrono::system_clock::now();
    if (now >= end_time_) {
        return std::chrono::seconds(0);
    }
    
    return std::chrono::duration_cast<std::chrono::seconds>(end_time_ - now);
}

std::vector<models::OrderPtr> TwapAlgorithm::getChildOrders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return child_orders_;
}

void TwapAlgorithm::scheduleNextSlice() {
    // This is handled by the Timer class in this implementation
}

void TwapAlgorithm::createAndSendSlice() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (state_ != State::RUNNING) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    if (now >= end_time_) {
        // We've reached the end time, check completion
        checkCompletion();
        return;
    }
    
    // Calculate remaining time and quantity
    auto remaining_time = std::chrono::duration_cast<std::chrono::seconds>(end_time_ - now);
    double remaining_quantity = config_.total_quantity - executed_quantity_;
    
    if (remaining_quantity <= 0) {
        // All quantity has been executed
        checkCompletion();
        return;
    }
    
    // Calculate number of slices remaining and slice size
    int remaining_slices = static_cast<int>(
        std::ceil(static_cast<double>(remaining_time.count()) / 
                  static_cast<double>(config_.slice_interval.count())));
    
    // Ensure at least one slice
    remaining_slices = std::max(1, remaining_slices);
    
    // Calculate slice size
    double slice_size = remaining_quantity / remaining_slices;
    
    // Create a new order for this slice
    auto order = std::make_shared<models::Order>();
    order->setTradingPair(trading_pair_);
    order->setSide(side_);
    
    if (config_.price_limit > 0) {
        order->setType(models::OrderType::LIMIT);
        order->setPrice(config_.price_limit);
    } else {
        order->setType(models::OrderType::MARKET);
    }
    
    order->setQuantity(slice_size);
    
    // Add to our child orders
    child_orders_.push_back(order);
    
    // Notify callback
    if (order_callback_) {
        order_callback_(order);
    }
}

void TwapAlgorithm::updateExecutionStatistics() {
    executed_quantity_ = 0.0;
    double total_filled_value = 0.0;
    
    for (const auto& order : child_orders_) {
        double filled_qty = order->getFilledQuantity();
        executed_quantity_ += filled_qty;
        total_filled_value += filled_qty * order->getAverageFillPrice();
    }
    
    if (executed_quantity_ > 0) {
        average_execution_price_ = total_filled_value / executed_quantity_;
    }
}

void TwapAlgorithm::checkCompletion() {
    auto now = std::chrono::system_clock::now();
    bool time_complete = now >= end_time_;
    double remaining_quantity = config_.total_quantity - executed_quantity_;
    bool quantity_complete = remaining_quantity <= 0.000001; // Small epsilon to handle floating point errors
    
    // Check if all orders are completed
    bool all_orders_complete = true;
    for (const auto& order : child_orders_) {
        auto state = order->getState();
        if (state != models::OrderState::FILLED &&
            state != models::OrderState::CANCELED &&
            state != models::OrderState::REJECTED) {
            all_orders_complete = false;
            break;
        }
    }
    
    // Algorithm is complete if we've reached the end time or executed all quantity
    // and all orders are in a terminal state
    if ((time_complete || quantity_complete) && all_orders_complete) {
        if (state_ != State::COMPLETED && state_ != State::CANCELLED) {
            state_ = State::COMPLETED;
            
            if (timer_) {
                timer_->stop();
            }
            
            // Notify completion
            if (completion_callback_) {
                completion_callback_(true, "Algorithm completed successfully");
            }
        }
    }
}

void TwapAlgorithm::setErrorState(const std::string& error_message) {
    state_ = State::ERROR;
    error_message_ = error_message;
    
    // Notify completion with error
    if (completion_callback_) {
        completion_callback_(false, error_message);
    }
}

} // namespace algorithms
} // namespace execution
} // namespace trading