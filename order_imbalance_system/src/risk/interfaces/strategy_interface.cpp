/**
 * Implementation of the interface for communication with the strategy engine
 *
 * File: strategy_interface.cpp
 * Created: 2025-04-29
 */

 #include "strategy_interface.hpp"
 #include <common/zeromq_client.hpp>
 #include <common/logger.hpp>

 #include <nlohmann/json.hpp>
 #include <sstream>
 #include <chrono>
 #include <thread>
 #include <mutex>
 namespace trading {
 namespace risk {
 
 // Implementation of the StrategyInterface that uses ZeroMQ for communication
 class StrategyInterfaceImpl : public StrategyInterface {
 private:
     // ZeroMQ client for communication
     std::unique_ptr<common::ZeroMQClient> zmqClient_;
     
     // ZeroMQ subscriber for strategy events
     std::unique_ptr<common::ZeroMQClient> zmqSubscriber_;
     
     // Callback for strategy events
     std::function<void(const StrategyId&, const std::string&)> strategyEventCallback_;
     
     // Mutex for thread safety
     std::mutex mutex_;
     
     // Configuration
     std::string strategyServiceEndpoint_;
     std::string strategyEventsTopic_;
     
     // Flag to indicate if initialization was successful
     bool initialized_ = false;
     
     // Thread for processing strategy events
     std::thread eventProcessingThread_;
     
     // Flag to indicate if the event processing thread should continue running
     bool running_ = false;
 
     // Process strategy events in a separate thread
     void processEvents() {
         while (running_) {
             try {
                 // Receive strategy events from the subscription
                 auto message = zmqSubscriber_->receiveMessage(100); // 100ms timeout
                 
                 if (!message.empty()) {
                     // Parse the received message
                     auto json = nlohmann::json::parse(message);
                     
                     StrategyId strategyId = json["strategy_id"];
                     std::string eventData = json["event_data"].dump();
                     
                     // Call the callback if registered
                     if (strategyEventCallback_) {
                         strategyEventCallback_(strategyId, eventData);
                     }
                 }
             } catch (const std::exception& e) {
                 LOG_ERROR("Error processing strategy events: {}", e.what());
             }
         }
     }
 
 public:
     StrategyInterfaceImpl() : StrategyInterface() {
         // Load configuration
         // In a real implementation, this would be loaded from a config file
         strategyServiceEndpoint_ = "tcp://localhost:5555";
         strategyEventsTopic_ = "strategy_events";
     }
     
     ~StrategyInterfaceImpl() override {
         // Stop the event processing thread
         if (running_) {
             running_ = false;
             if (eventProcessingThread_.joinable()) {
                 eventProcessingThread_.join();
             }
         }
     }
     
     bool initialize() override {
         std::lock_guard<std::mutex> lock(mutex_);
         
         if (initialized_) {
             return true;
         }
         
         try {
             // Initialize ZeroMQ client for request-reply
             zmqClient_ = std::make_unique<common::ZeroMQClient>();
             if (!zmqClient_->connect(strategyServiceEndpoint_, common::ZeroMQClient::REQ)) {
                 LOG_ERROR("Failed to connect to strategy service at {}", strategyServiceEndpoint_);
                 return false;
             }
             
             // Initialize ZeroMQ subscriber for events
             zmqSubscriber_ = std::make_unique<common::ZeroMQClient>();
             if (!zmqSubscriber_->connect(strategyServiceEndpoint_ + "_pub", common::ZeroMQClient::SUB)) {
                 LOG_ERROR("Failed to connect to strategy events at {}", strategyServiceEndpoint_ + "_pub");
                 return false;
             }
             
             // Subscribe to the strategy events topic
             zmqSubscriber_->subscribe(strategyEventsTopic_);
             
             // Start the event processing thread
             running_ = true;
             eventProcessingThread_ = std::thread(&StrategyInterfaceImpl::processEvents, this);
             
             initialized_ = true;
             LOG_INFO("Strategy interface initialized successfully");
             return true;
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to initialize strategy interface: {}", e.what());
             return false;
         }
     }
     
     bool sendRiskFeedback(const StrategyId& strategyId, const std::string& feedback) override {
         if (!initialized_) {
             LOG_ERROR("Strategy interface not initialized");
             return false;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "risk_feedback"},
                 {"strategy_id", strategyId},
                 {"feedback", nlohmann::json::parse(feedback)}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             return responseJson["success"].get<bool>();
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to send risk feedback: {}", e.what());
             return false;
         }
     }
     
     std::string getStrategyParameters(const StrategyId& strategyId) override {
         if (!initialized_) {
             LOG_ERROR("Strategy interface not initialized");
             return "{}";
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_parameters"},
                 {"strategy_id", strategyId}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["parameters"].dump();
             } else {
                 LOG_ERROR("Failed to get strategy parameters: {}", responseJson["error"].get<std::string>());
                 return "{}";
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get strategy parameters: {}", e.what());
             return "{}";
         }
     }
     
     bool updateStrategyParameters(const StrategyId& strategyId, const std::string& parameters) override {
         if (!initialized_) {
             LOG_ERROR("Strategy interface not initialized");
             return false;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "update_parameters"},
                 {"strategy_id", strategyId},
                 {"parameters", nlohmann::json::parse(parameters)}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             return responseJson["success"].get<bool>();
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to update strategy parameters: {}", e.what());
             return false;
         }
     }
     
     bool notifyRiskEvent(const StrategyId& strategyId, const RiskEvent& event) override {
         if (!initialized_) {
             LOG_ERROR("Strategy interface not initialized");
             return false;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "risk_event"},
                 {"strategy_id", strategyId},
                 {"event_type", event.type},
                 {"event_severity", event.severity},
                 {"event_message", event.message},
                 {"event_data", nlohmann::json::parse(event.data)}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             return responseJson["success"].get<bool>();
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to notify risk event: {}", e.what());
             return false;
         }
     }
     
     std::vector<StrategyId> getActiveStrategies() override {
         if (!initialized_) {
             LOG_ERROR("Strategy interface not initialized");
             return {};
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_active_strategies"}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 std::vector<StrategyId> strategies;
                 for (const auto& strategy : responseJson["strategies"]) {
                     strategies.push_back(strategy.get<StrategyId>());
                 }
                 return strategies;
             } else {
                 LOG_ERROR("Failed to get active strategies: {}", responseJson["error"].get<std::string>());
                 return {};
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get active strategies: {}", e.what());
             return {};
         }
     }
     
     bool isStrategyActive(const StrategyId& strategyId) override {
         if (!initialized_) {
             LOG_ERROR("Strategy interface not initialized");
             return false;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "is_strategy_active"},
                 {"strategy_id", strategyId}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["active"].get<bool>();
             } else {
                 LOG_ERROR("Failed to check if strategy is active: {}", responseJson["error"].get<std::string>());
                 return false;
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to check if strategy is active: {}", e.what());
             return false;
         }
     }
     
     void registerStrategyEventCallback(const std::function<void(const StrategyId&, const std::string&)>& callback) override {
         std::lock_guard<std::mutex> lock(mutex_);
         strategyEventCallback_ = callback;
     }
 };
 
 // Factory function to create StrategyInterface
 StrategyInterface::StrategyInterface() {
     // Base constructor, nothing to do here
 }
 
 // Global factory function to create the concrete implementation
 std::unique_ptr<StrategyInterface> createStrategyInterface() {
     return std::make_unique<StrategyInterfaceImpl>();
 }
 
 } // namespace risk
}