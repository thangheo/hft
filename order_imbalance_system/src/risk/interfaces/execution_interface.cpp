/**
 * Implementation of the interface for communication with the execution engine
 *
 * File: execution_interface.cpp
 * Created: 2025-04-29
 */

 #include "execution_interface.hpp"
 #include <common/zeromq_client.hpp>
 #include <common/logger.hpp>

 #include <nlohmann/json.hpp>
 #include <sstream>
 #include <chrono>
 #include <thread>
 #include <mutex>
 
 namespace trading {
 namespace risk {
 
 // Implementation of the ExecutionInterface that uses ZeroMQ for communication
 class ExecutionInterfaceImpl : public ExecutionInterface {
 private:
     // ZeroMQ client for communication
     std::unique_ptr<common::ZeroMQClient> zmqClient_;
     
     // ZeroMQ subscriber for execution reports
     std::unique_ptr<common::ZeroMQClient> zmqSubscriber_;
     
     // Callback for execution reports
     std::function<void(const ExecutionReport&)> executionReportCallback_;
     
     // Mutex for thread safety
     std::mutex mutex_;
     
     // Configuration
     std::string executionServiceEndpoint_;
     std::string executionReportsTopic_;
     
     // Flag to indicate if initialization was successful
     bool initialized_ = false;
     
     // Thread for processing execution reports
     std::thread reportProcessingThread_;
     
     // Flag to indicate if the report processing thread should continue running
     bool running_ = false;
 
     // Process execution reports in a separate thread
     void processReports() {
         while (running_) {
             try {
                 // Receive execution reports from the subscription
                 auto message = zmqSubscriber_->receiveMessage(100); // 100ms timeout
                 
                 if (!message.empty()) {
                     // Parse the received message
                     auto json = nlohmann::json::parse(message);
                     
                     // Create ExecutionReport from JSON
                     ExecutionReport report;
                     report.orderId = json["order_id"];
                     report.exchangeOrderId = json["exchange_order_id"];
                     report.exchangeId = json["exchangeId"];
                     report.tradingPair = json["trading_pair"];
                     report.side = json["side"] == "buy" ? OrderSide::BUY : OrderSide::SELL;
                     report.quantity = json["quantity"];
                     report.price = json["price"];
                     report.status = json["status"];
                     report.filledQuantity = json["filled_quantity"];
                     report.averageFillPrice = json["average_fill_price"];
                     report.timestamp = json["timestamp"];
                     report.fee = json["fee"];
                     report.fee_currency = json["fee_currency"];
                     
                     // Call the callback if registered
                     if (executionReportCallback_) {
                         executionReportCallback_(report);
                     }
                 }
             } catch (const std::exception& e) {
                 LOG_ERROR("Error processing execution reports: {}", e.what());
             }
         }
     }
 
     // Convert OrderRequest to JSON
     nlohmann::json orderRequestToJson(const OrderRequest& request) {
         nlohmann::json json;
         json["order_id"] = request.orderId;
         json["client_order_id"] = request.clientOrderId;
         json["exchangeId"] = request.exchangeId;
         json["trading_pair"] = request.tradingPair;
         json["side"] = request.side == OrderSide::BUY ? "buy" : "sell";
         json["type"] = request.type;
         json["quantity"] = request.quantity;
         json["price"] = request.price;
         json["time_in_force"] = request.timeInForce;
         json["strategy_id"] = request.strategyId;
         json["signal_id"] = request.signalId;
         json["timestamp"] = request.timestamp;
         
         // Add optional parameters if provided
         if (!request.params.empty()) {
             json["params"] = nlohmann::json::object();
             for (const auto& param : request.params) {
                 json["params"][param.first] = param.second;
             }
         }
         
         return json;
     }
 
     // Parse OrderRequest from JSON
     OrderRequest jsonToOrderRequest(const nlohmann::json& json) {
         OrderRequest request;
         request.orderId = json["order_id"];
         request.clientOrderId = json["client_order_id"];
         request.exchangeId = json["exchangeId"];
         request.tradingPair = json["trading_pair"];
         request.side = json["side"] == "buy" ? OrderSide::BUY : OrderSide::SELL;
         request.type = json["type"];
         request.quantity = json["quantity"];
         request.price = json["price"];
         request.timeInForce = json["time_in_force"];
         request.strategyId = json["strategy_id"];
         request.signalId = json["signal_id"];
         request.timestamp = json["timestamp"];
         
         // Parse optional parameters if provided
         if (json.contains("params") && json["params"].is_object()) {
             for (auto it = json["params"].begin(); it != json["params"].end(); ++it) {
                 request.params[it.key()] = it.value();
             }
         }
         
         return request;
     }
 
 public:
     ExecutionInterfaceImpl() : ExecutionInterface() {
         // Load configuration
         // In a real implementation, this would be loaded from a config file
         executionServiceEndpoint_ = "tcp://localhost:5556";
         executionReportsTopic_ = "execution_reports";
     }
     
     ~ExecutionInterfaceImpl() override {
         // Stop the report processing thread
         if (running_) {
             running_ = false;
             if (reportProcessingThread_.joinable()) {
                 reportProcessingThread_.join();
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
             if (!zmqClient_->connect(executionServiceEndpoint_, common::ZeroMQClient::REQ)) {
                 LOG_ERROR("Failed to connect to execution service at {}", executionServiceEndpoint_);
                 return false;
             }
             
             // Initialize ZeroMQ subscriber for reports
             zmqSubscriber_ = std::make_unique<common::ZeroMQClient>();
             if (!zmqSubscriber_->connect(executionServiceEndpoint_ + "_pub", common::ZeroMQClient::SUB)) {
                 LOG_ERROR("Failed to connect to execution reports at {}", executionServiceEndpoint_ + "_pub");
                 return false;
             }
             
             // Subscribe to the execution reports topic
             zmqSubscriber_->subscribe(executionReportsTopic_);
             
             // Start the report processing thread
             running_ = true;
             reportProcessingThread_ = std::thread(&ExecutionInterfaceImpl::processReports, this);
             
             initialized_ = true;
             LOG_INFO("Execution interface initialized successfully");
             return true;
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to initialize execution interface: {}", e.what());
             return false;
         }
     }
     
     std::string submitOrder(const OrderRequest& orderRequest) override {
         if (!initialized_) {
             LOG_ERROR("Execution interface not initialized");
             return "";
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "submit_order"},
                 {"order", orderRequestToJson(orderRequest)}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["order_id"].get<std::string>();
             } else {
                 LOG_ERROR("Failed to submit order: {}", responseJson["error"].get<std::string>());
                 return "";
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to submit order: {}", e.what());
             return "";
         }
     }
     
     bool cancelOrder(const std::string& orderId) override {
         if (!initialized_) {
             LOG_ERROR("Execution interface not initialized");
             return false;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "cancel_order"},
                 {"order_id", orderId}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             return responseJson["success"].get<bool>();
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to cancel order: {}", e.what());
             return false;
         }
     }
     
     int cancelAllOrders(const ExchangeId& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Execution interface not initialized");
             return 0;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "cancel_all_orders"},
                 {"exchangeId", exchangeId}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["cancelled_count"].get<int>();
             } else {
                 LOG_ERROR("Failed to cancel all orders: {}", responseJson["error"].get<std::string>());
                 return 0;
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to cancel all orders: {}", e.what());
             return 0;
         }
     }
     
     int cancelAllOrders(const ExchangeId& exchangeId, const TradingPair& tradingPair) override {
         if (!initialized_) {
             LOG_ERROR("Execution interface not initialized");
             return 0;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "cancel_all_orders"},
                 {"exchangeId", exchangeId},
                 {"trading_pair", tradingPair}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["cancelled_count"].get<int>();
             } else {
                 LOG_ERROR("Failed to cancel all orders: {}", responseJson["error"].get<std::string>());
                 return 0;
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to cancel all orders: {}", e.what());
             return 0;
         }
     }
     
     std::string getOrderStatus(const std::string& orderId) override {
         if (!initialized_) {
             LOG_ERROR("Execution interface not initialized");
             return "{}";
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_order_status"},
                 {"order_id", orderId}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["order_status"].dump();
             } else {
                 LOG_ERROR("Failed to get order status: {}", responseJson["error"].get<std::string>());
                 return "{}";
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get order status: {}", e.what());
             return "{}";
         }
     }
     
     void registerExecutionReportCallback(const std::function<void(const ExecutionReport&)>& callback) override {
         std::lock_guard<std::mutex> lock(mutex_);
         executionReportCallback_ = callback;
     }
     
     std::vector<OrderRequest> getOpenOrders(const std::optional<ExchangeId>& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Execution interface not initialized");
             return {};
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_open_orders"}
             };
             
             // Add exchange ID if provided
             if (exchangeId) {
                 request["exchangeId"] = *exchangeId;
             }
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 std::vector<OrderRequest> orders;
                 for (const auto& orderJson : responseJson["orders"]) {
                     orders.push_back(jsonToOrderRequest(orderJson));
                 }
                 return orders;
             } else {
                 LOG_ERROR("Failed to get open orders: {}", responseJson["error"].get<std::string>());
                 return {};
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get open orders: {}", e.what());
             return {};
         }
     }
     
     bool isExchangeAvailable(const ExchangeId& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Execution interface not initialized");
             return false;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "is_exchange_available"},
                 {"exchangeId", exchangeId}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["available"].get<bool>();
             } else {
                 LOG_ERROR("Failed to check if exchange is available: {}", responseJson["error"].get<std::string>());
                 return false;
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to check if exchange is available: {}", e.what());
             return false;
         }
     }
     
     std::string getExchangeStatus(const ExchangeId& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Execution interface not initialized");
             return "{}";
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_exchange_status"},
                 {"exchangeId", exchangeId}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["exchange_status"].dump();
             } else {
                 LOG_ERROR("Failed to get exchange status: {}", responseJson["error"].get<std::string>());
                 return "{}";
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get exchange status: {}", e.what());
             return "{}";
         }
     }
 };
 
 // Factory function to create ExecutionInterface
 ExecutionInterface::ExecutionInterface() {
     // Base constructor, nothing to do here
 }
 
 // Global factory function to create the concrete implementation
 std::unique_ptr<ExecutionInterface> createExecutionInterface() {
     return std::make_unique<ExecutionInterfaceImpl>();
 }
 
 } // namespace risk
}