/**
 * @file pov_algorithm.cpp
 * @brief Implementation of Percentage of Volume (POV) execution algorithm
 * 
 * @details 
 * This algorithm executes large orders by participating in the market 
 * at a specified percentage of market volume to minimize market impact.
 * 
 * Created on: May 9, 2025
 */
 #include "execution/algorithms/timer.hpp"
 #include "execution/algorithms/pov_algorithm.hpp"
 #include <algorithm>
 #include <cmath>
 #include <sstream>
 #include <iomanip>
 
 namespace trading {
 namespace execution {
 namespace algorithms {
 
 PovAlgorithm::PovAlgorithm() : timer_(std::make_unique<Timer>()) {}
 
 bool PovAlgorithm::initialize(
     const std::string& trading_pair,
     models::OrderSide side,
     const PovConfig& config) {
     
     std::lock_guard<std::mutex> lock(mutex_);
     
     if (state_ != State::INITIALIZED && state_ != State::ERROR && state_ != State::COMPLETED) {
         return false; // Can only initialize from certain states
     }
     
     // Validate configuration
     if (config.total_quantity <= 0.0) {
         setErrorState("Total quantity must be positive");
         return false;
     }
     
     if (config.participation_rate <= 0.0 || config.participation_rate > 1.0) {
         setErrorState("Participation rate must be between 0 and 1");
         return false;
     }
     
     if (config.duration.count() <= 0) {
         setErrorState("Duration must be positive");
         return false;
     }
     
     // Store parameters
     trading_pair_ = trading_pair;
     side_ = side;
     config_ = config;
     executed_quantity_ = 0.0;
     average_execution_price_ = 0.0;
     recent_volumes_.clear();
     cumulative_market_volume_ = 0.0;
     target_participation_rate_ = config.participation_rate;
     child_orders_.clear();
     
     // Reset time tracking
     start_time_ = std::chrono::system_clock::time_point();
     end_time_ = std::chrono::system_clock::time_point();
     last_volume_check_ = std::chrono::system_clock::time_point();
     
     state_ = State::INITIALIZED;
     return true;
 }
 
 bool PovAlgorithm::start() {
     std::lock_guard<std::mutex> lock(mutex_);
     
     if (state_ != State::INITIALIZED) {
         return false; // Can only start from initialized state
     }
     
     if (!execution_service_) {
         setErrorState("Execution service not set");
         return false;
     }
     
     // Set start and end times
     start_time_ = std::chrono::system_clock::now();
     end_time_ = start_time_ + config_.duration;
     last_volume_check_ = start_time_;
     
     // Initialize market volume tracking
     checkMarketVolume();
     
     // Schedule regular volume checks and order placement
     timer_->setCallback([this]() {
         checkMarketVolume();
         calculateOrderSize();
     });
     timer_->setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(config_.check_interval));
     timer_->start();
     
     state_ = State::RUNNING;
     return true;
 }
 
 bool PovAlgorithm::pause() {
     std::lock_guard<std::mutex> lock(mutex_);
     
     if (state_ != State::RUNNING) {
         return false; // Can only pause when running
     }
     
     timer_->stop();
     state_ = State::PAUSED;
     return true;
 }
 
 bool PovAlgorithm::cancel() {
     std::lock_guard<std::mutex> lock(mutex_);
     
     if (state_ != State::RUNNING && state_ != State::PAUSED) {
         return false; // Can only cancel when running or paused
     }
     
     timer_->stop();
     
     // Cancel any open child orders
     for (const auto& order : child_orders_) {
         if (order->isActive()) {
             execution_service_->cancelOrder(order->getClientOrderId());
         }
     }
     
     state_ = State::CANCELLED;
     
     // Notify completion
     if (completion_callback_) {
         completion_callback_(false, "Algorithm cancelled");
     }
     
     return true;
 }
 
 ExecutionResponse PovAlgorithm::execute(const models::Order& parentOrder, ExecutionService& execution_service) {
     // Extract configuration from parent order
     PovConfig extracted_config = extractConfigFromOrder(parentOrder);
     
     // Store execution service reference
     execution_service_ = &execution_service;
     
     // Initialize algorithm
     bool init_result = initialize(
         parentOrder.getTradingPair(),
         parentOrder.getSide(),
         extracted_config
     );
     
     if (!init_result) {
         return ExecutionResponse {
             parentOrder.getClientOrderId(),
             "",
             false,
             error_message_,
             "ERROR",
             std::chrono::system_clock::now()
         };
     }
     
     // Start execution
     bool start_result = start();
     
     if (!start_result) {
         return ExecutionResponse {
             parentOrder.getClientOrderId(),
             "",
             false,
             error_message_,
             "ERROR",
             std::chrono::system_clock::now()
         };
     }
     
     return ExecutionResponse {
         parentOrder.getClientOrderId(),
         "",
         true,
         "",
         "RUNNING",
         std::chrono::system_clock::now()
     };
 }
 
 PovConfig PovAlgorithm::extractConfigFromOrder(const models::Order& parentOrder) {
     PovConfig config;
     
     // Extract basic configuration
     config.total_quantity = parentOrder.getQuantity();
     config.exchange_id = parentOrder.getExchangeId();
     
     // Extract POV-specific parameters from extra_params
     const auto& extra_params = parentOrder.getExtraParams();
     
     // Extract participation rate (required)
     auto it = extra_params.find("participation_rate");
     if (it != extra_params.end()) {
         config.participation_rate = std::stod(it->second);
     } else {
         // Default to conservative 5% if not specified
         config.participation_rate = 0.05;
     }
     
     // Extract duration (required)
     it = extra_params.find("duration_seconds");
     if (it != extra_params.end()) {
         config.duration = std::chrono::seconds(std::stoi(it->second));
     } else {
         // Default to 1 hour if not specified
         config.duration = std::chrono::hours(1);
     }
     
     // Extract optional parameters
     it = extra_params.find("min_order_size");
     if (it != extra_params.end()) {
         config.min_order_size = std::stod(it->second);
     }
     
     it = extra_params.find("max_order_size");
     if (it != extra_params.end()) {
         config.max_order_size = std::stod(it->second);
     }
     
     it = extra_params.find("check_interval_seconds");
     if (it != extra_params.end()) {
         config.check_interval = std::chrono::seconds(std::stoi(it->second));
     }
     
     it = extra_params.find("price_limit");
     if (it != extra_params.end()) {
         config.price_limit = std::stod(it->second);
     }
     
     it = extra_params.find("deviation_pct");
     if (it != extra_params.end()) {
         config.deviation_pct = std::stod(it->second);
     }
     
     it = extra_params.find("dynamic_rate");
     if (it != extra_params.end()) {
         config.dynamic_rate = (it->second == "true" || it->second == "1");
     }
     
     it = extra_params.find("volume_lookback_minutes");
     if (it != extra_params.end()) {
         config.volume_lookback = std::chrono::minutes(std::stoi(it->second));
     }
     
     it = extra_params.find("urgency_factor");
     if (it != extra_params.end()) {
         config.urgency_factor = std::stod(it->second);
     }
     
     return config;
 }
 
 void PovAlgorithm::setOrderCallback(const OrderCallback& callback) {
     std::lock_guard<std::mutex> lock(mutex_);
     order_callback_ = callback;
 }
 
 void PovAlgorithm::setCompletionCallback(const CompletionCallback& callback) {
     std::lock_guard<std::mutex> lock(mutex_);
     completion_callback_ = callback;
 }
 
 void PovAlgorithm::setMarketVolumeCallback(const MarketVolumeCallback& callback) {
     std::lock_guard<std::mutex> lock(mutex_);
     market_volume_callback_ = callback;
 }
 
 void PovAlgorithm::handleOrderUpdate(const models::OrderPtr& order) {
     std::lock_guard<std::mutex> lock(mutex_);
     
     // Find the order in our child orders
     auto it = std::find_if(child_orders_.begin(), child_orders_.end(),
         [&order](const models::OrderPtr& child) {
             return child->getClientOrderId() == order->getClientOrderId();
         });
     
     if (it != child_orders_.end()) {
         // Update our copy of the order
         *it = order;
         
         // Update execution statistics
         updateExecutionStatistics();
         
         // Check if we're done
         checkCompletion();
     }
 }
 
 bool PovAlgorithm::isRunning() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return state_ == State::RUNNING;
 }
 
 bool PovAlgorithm::isComplete() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return state_ == State::COMPLETED;
 }
 
 bool PovAlgorithm::isCancelled() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return state_ == State::CANCELLED;
 }
 
 double PovAlgorithm::getTotalQuantity() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return config_.total_quantity;
 }
 
 double PovAlgorithm::getExecutedQuantity() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return executed_quantity_;
 }
 
 double PovAlgorithm::getRemainingQuantity() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return config_.total_quantity - executed_quantity_;
 }
 
 double PovAlgorithm::getAverageExecutionPrice() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return average_execution_price_;
 }
 
 double PovAlgorithm::getParticipationRate() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return target_participation_rate_;
 }
 
 std::chrono::seconds PovAlgorithm::getRemainingTime() const {
     std::lock_guard<std::mutex> lock(mutex_);
     
     if (state_ != State::RUNNING && state_ != State::PAUSED) {
         return std::chrono::seconds(0);
     }
     
     auto now = std::chrono::system_clock::now();
     if (now >= end_time_) {
         return std::chrono::seconds(0);
     }
     
     return std::chrono::duration_cast<std::chrono::seconds>(end_time_ - now);
 }
 
 std::vector<models::OrderPtr> PovAlgorithm::getChildOrders() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return child_orders_;
 }
 
 void PovAlgorithm::checkMarketVolume() {
     if (!market_volume_callback_) {
         return; // No way to check market volume
     }
     
     // Get market volume since last check
     auto now = std::chrono::system_clock::now();
     auto time_since_last_check = std::chrono::duration_cast<std::chrono::minutes>(
         now - last_volume_check_);
     
     if (time_since_last_check.count() <= 0) {
         return; // Don't check too frequently
     }
     
     // Get volume for the time period
     double volume = market_volume_callback_(trading_pair_, time_since_last_check);
     
     // Store in recent volumes and update cumulative
     recent_volumes_.push_back(volume);
     cumulative_market_volume_ += volume;
     
     // Keep only volumes within the lookback window
     while (recent_volumes_.size() > 1) {
         auto oldest_volume_time = last_volume_check_ - 
             std::chrono::duration_cast<std::chrono::system_clock::duration>(config_.volume_lookback);
         
         if (oldest_volume_time <= start_time_) {
             break; // Don't go before algorithm start
         }
         
         // Remove oldest volume
         cumulative_market_volume_ -= recent_volumes_.front();
         recent_volumes_.pop_front();
     }
     
     // Update last check time
     last_volume_check_ = now;
     
     // If using dynamic rate, adjust participation rate based on time remaining
     if (config_.dynamic_rate && state_ == State::RUNNING) {
         auto time_remaining = std::chrono::duration_cast<std::chrono::seconds>(end_time_ - now).count();
         auto total_time = std::chrono::duration_cast<std::chrono::seconds>(config_.duration).count();
         
         if (time_remaining > 0 && total_time > 0) {
             // Calculate how far along we are in terms of time (0-1)
             double time_progress = 1.0 - (static_cast<double>(time_remaining) / total_time);
             
             // Calculate how far along we are in terms of quantity (0-1)
             double quantity_progress = executed_quantity_ / config_.total_quantity;
             
             // If we're behind or ahead of schedule, adjust participation rate
             double progress_difference = time_progress - quantity_progress;
             
             // Adjust by at most 50% of the configured rate
             double max_adjustment = config_.participation_rate * 0.5;
             double adjustment = progress_difference * max_adjustment * config_.urgency_factor;
             
             target_participation_rate_ = std::max(
                 config_.participation_rate * 0.5,
                 std::min(
                     config_.participation_rate * 1.5,
                     config_.participation_rate + adjustment
                 )
             );
         }
     }
 }
 
 void PovAlgorithm::calculateOrderSize() {
     if (state_ != State::RUNNING) {
         return;
     }
     
     // Check if we have any volume data
     if (cumulative_market_volume_ <= 0.0) {
         return;
     }
     
     // Calculate optimal order size
     double order_size = calculateOptimalOrderSize();
     
     // Check constraints
     double remaining = config_.total_quantity - executed_quantity_;
     double min_size = config_.min_order_size;
     double max_size = config_.max_order_size > 0.0 ? 
         std::min(config_.max_order_size, remaining) : remaining;
     
     // Apply constraints
     if (order_size < min_size) {
         if (remaining < min_size) {
             // Less than minimum remains, execute all
             order_size = remaining;
         } else {
             // Use minimum size
             order_size = min_size;
         }
     } else if (order_size > max_size) {
         order_size = max_size;
     }
     
     // Round to appropriate precision (assuming 8 decimal places for crypto)
     order_size = std::floor(order_size * 100000000.0) / 100000000.0;
     
     // Check if order size is significant
     if (order_size > 0.0) {
         placeChildOrder(order_size);
     }
 }
 
 double PovAlgorithm::calculateOptimalOrderSize() {
     auto now = std::chrono::system_clock::now();
     
     // Project expected volume until the end of the algorithm
     double expected_total_volume = cumulative_market_volume_;
     
     // Only project forward if we have some volume history
     if (!recent_volumes_.empty() && now < end_time_) {
         // Calculate average volume per minute
         double total_minutes = std::chrono::duration_cast<std::chrono::minutes>(
             last_volume_check_ - start_time_).count();
         
         if (total_minutes > 0) {
             double volume_per_minute = cumulative_market_volume_ / total_minutes;
             
             // Project remaining minutes
             double remaining_minutes = std::chrono::duration_cast<std::chrono::minutes>(
                 end_time_ - now).count();
             
             expected_total_volume += volume_per_minute * remaining_minutes;
         }
     }
     
     // Calculate target quantity based on participation rate
     double target_total_execution = expected_total_volume * target_participation_rate_;
     
     // Calculate how much more we need to execute
     double target_remaining = target_total_execution - executed_quantity_;
     
     // Calculate how much to execute now
     double order_size = target_remaining * (
         std::chrono::duration_cast<std::chrono::seconds>(config_.check_interval).count() / 
         std::chrono::duration_cast<std::chrono::seconds>(end_time_ - now).count()
     );
     
     return order_size;
 }
 
 void PovAlgorithm::placeChildOrder(double quantity) {
     if (!execution_service_) {
         return;
     }
     
     // Create a new order
     auto order = std::make_shared<models::Order>();
     order->setTradingPair(trading_pair_);
     order->setSide(side_);
     order->setQuantity(quantity);
     
     // Determine order type and price
     if (config_.price_limit > 0.0) {
         order->setType(models::OrderType::LIMIT);
         order->setPrice(config_.price_limit);
     } else {
         order->setType(models::OrderType::MARKET);
     }
     
     // Set exchange if specified
     if (!config_.exchange_id.empty()) {
         order->setExchangeId(config_.exchange_id);
     }
     
     // Set time in force
     order->setTimeInForce(models::TimeInForce::IOC); // Immediate-or-cancel to avoid lingering orders
     
     // Add POV algorithm identifier for tracking
     order->setExtraParam("algorithm", "POV");
     
     // Add parent order identifier if available
     // TBD: We likely need a way to track the parent order ID here
     
     // Place the order
     auto response = execution_service_->placeOrder(*order);
     
     if (response.success) {
         // Update order with exchange order ID if available
         if (!response.exchange_order_id.empty()) {
             order->setExchangeOrderId(response.exchange_order_id);
         }
         
         // Add to child orders
         child_orders_.push_back(order);
         
         // Notify via callback
         if (order_callback_) {
             order_callback_(order);
         }
     }
 }
 
 void PovAlgorithm::updateExecutionStatistics() {
     // Reset statistics
     executed_quantity_ = 0.0;
     double total_value = 0.0;
     
     // Recalculate from all child orders
     for (const auto& order : child_orders_) {
         double filled = order->getFilledQuantity();
         executed_quantity_ += filled;
         total_value += filled * order->getAverageFillPrice();
     }
     
     // Calculate average execution price
     if (executed_quantity_ > 0.0) {
         average_execution_price_ = total_value / executed_quantity_;
     }
 }
 
 void PovAlgorithm::checkCompletion() {
     if (state_ != State::RUNNING) {
         return;
     }
     
     auto now = std::chrono::system_clock::now();
     bool time_expired = now >= end_time_;
     bool fully_executed = std::abs(executed_quantity_ - config_.total_quantity) < 1e-8;
     
     if (time_expired || fully_executed) {
         // Stop timer
         timer_->stop();
         
         // Cancel any open child orders
         for (const auto& order : child_orders_) {
             if (order->isActive()) {
                 execution_service_->cancelOrder(order->getClientOrderId());
             }
         }
         
         // Mark as completed
         state_ = State::COMPLETED;
         
         // Notify completion
         if (completion_callback_) {
             std::ostringstream message;
             message << "Algorithm completed. Executed " 
                     << std::fixed << std::setprecision(8) << executed_quantity_ 
                     << " of " << config_.total_quantity << " ("
                     << std::setprecision(2) << (executed_quantity_ / config_.total_quantity * 100.0) 
                     << "%)";
                     
             completion_callback_(true, message.str());
         }
     }
 }
 
 void PovAlgorithm::setErrorState(const std::string& error_message) {
     state_ = State::ERROR;
     error_message_ = error_message;
 }
 
 double PovAlgorithm::getMarketVolumeForPeriod(std::chrono::minutes period) {
     if (!market_volume_callback_) {
         return 0.0;
     }
     
     return market_volume_callback_(trading_pair_, period);
 }
 
 } // namespace algorithms
 } // namespace execution
 } // namespace trading