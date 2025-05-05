/**
 * Implementation of the interface for accessing market data
 *
 * File: data_interface.cpp
 * Created: 2025-04-29
 */

 #include "data_interface.hpp"
 #include <common/zeromq_client.hpp>
 #include <common/logger.hpp>
 #include <nlohmann/json.hpp>
 #include <sstream>
 #include <chrono>
 #include <thread>
 #include <mutex>
 
 namespace trading {
 namespace risk {
 
 // Implementation of the DataInterface that uses ZeroMQ for communication
 class DataInterfaceImpl : public DataInterface {
 private:
     // ZeroMQ client for communication
     std::unique_ptr<common::ZeroMQClient> zmqClient_;
     
     // ZeroMQ subscriber for market data updates
     std::unique_ptr<common::ZeroMQClient> zmqSubscriber_;
     
     // Callback for market data updates
     std::function<void(const MarketData&)> marketDataCallback_;
     
     // Mutex for thread safety
     std::mutex mutex_;
     
     // Configuration
     std::string dataServiceEndpoint_;
     std::string marketDataTopic_;
     
     // Flag to indicate if initialization was successful
     bool initialized_ = false;
     
     // Thread for processing market data updates
     std::thread dataProcessingThread_;
     
     // Flag to indicate if the data processing thread should continue running
     bool running_ = false;
     
     // Active subscriptions
     std::map<std::string, bool> activeSubscriptions_;
 
     // Generate a subscription key
     std::string generateSubscriptionKey(const TradingPair& tradingPair, const std::optional<ExchangeId>& exchangeId) {
         std::string key = tradingPair;
         if (exchangeId) {
             key += ":" + *exchangeId;
         }
         return key;
     }
 
     // Process market data updates in a separate thread
     void processMarketData() {
         while (running_) {
             try {
                 // Receive market data from the subscription
                 auto message = zmqSubscriber_->receiveMessage(100); // 100ms timeout
                 
                 if (!message.empty()) {
                     // Parse the received message
                     auto json = nlohmann::json::parse(message);
                     
                     // Create MarketData from JSON
                     MarketData marketData;
                     marketData.tradingPair = json["trading_pair"];
                     marketData.exchangeId = json["exchange_id"];
                     marketData.timestamp = json["timestamp"];
                     marketData.price = json["price"];
                     marketData.volume = json["volume"];
                     marketData.bid = json["bid"];
                     marketData.ask = json["ask"];
                     marketData.lastTradePrice = json["last_trade_price"];
                     marketData.lastTradeQuantity = json["last_trade_quantity"];
                     
                     // Call the callback if registered
                     if (marketDataCallback_) {
                         marketDataCallback_(marketData);
                     }
                 }
             } catch (const std::exception& e) {
                 LOG_ERROR("Error processing market data: {}", e.what());
             }
         }
     }
 
     // Convert OrderRequest to JSON
     nlohmann::json orderRequestToJson(const OrderRequest& request) {
         nlohmann::json json;
         json["order_id"] = request.orderId;
         json["client_order_id"] = request.clientOrderId;
         json["exchange_id"] = request.exchangeId;
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
         request.exchangeId = json["exchange_id"];
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
     DataInterfaceImpl() : DataInterface() {
         // Load configuration
         // In a real implementation, this would be loaded from a config file
         dataServiceEndpoint_ = "tcp://localhost:5557";
         marketDataTopic_ = "market_data";
     }
     
     ~DataInterfaceImpl() override {
         // Stop the data processing thread
         if (running_) {
             running_ = false;
             if (dataProcessingThread_.joinable()) {
                 dataProcessingThread_.join();
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
             if (!zmqClient_->connect(dataServiceEndpoint_, common::ZeroMQClient::REQ)) {
                 LOG_ERROR("Failed to connect to data service at {}", dataServiceEndpoint_);
                 return false;
             }
             
             // Initialize ZeroMQ subscriber for market data updates
             zmqSubscriber_ = std::make_unique<common::ZeroMQClient>();
             if (!zmqSubscriber_->connect(dataServiceEndpoint_ + "_pub", common::ZeroMQClient::SUB)) {
                 LOG_ERROR("Failed to connect to market data at {}", dataServiceEndpoint_ + "_pub");
                 return false;
             }
             
             // Subscribe to the market data topic
             zmqSubscriber_->subscribe(marketDataTopic_);
             
             // Start the data processing thread
             running_ = true;
             dataProcessingThread_ = std::thread(&DataInterfaceImpl::processMarketData, this);
             
             initialized_ = true;
             LOG_INFO("Data interface initialized successfully");
             return true;
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to initialize data interface: {}", e.what());
             return false;
         }
     }
     
     MarketData getMarketData(const TradingPair& tradingPair, const std::optional<ExchangeId>& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return {};
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_market_data"},
                 {"trading_pair", tradingPair}
             };
             
             // Add exchange ID if provided
             if (exchangeId) {
                 request["exchange_id"] = *exchangeId;
             }
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 MarketData marketData;
                 marketData.tradingPair = responseJson["data"]["trading_pair"];
                 marketData.exchangeId = responseJson["data"]["exchange_id"];
                 marketData.timestamp = responseJson["data"]["timestamp"];
                 marketData.price = responseJson["data"]["price"];
                 marketData.volume = responseJson["data"]["volume"];
                 marketData.bid = responseJson["data"]["bid"];
                 marketData.ask = responseJson["data"]["ask"];
                 marketData.lastTradePrice = responseJson["data"]["last_trade_price"];
                 marketData.lastTradeQuantity = responseJson["data"]["last_trade_quantity"];
                 return marketData;
             } else {
                 LOG_ERROR("Failed to get market data: {}", responseJson["error"].get<std::string>());
                 return {};
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get market data: {}", e.what());
             return {};
         }
     }
     
     double getCurrentPrice(const TradingPair& tradingPair, const std::optional<ExchangeId>& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return 0.0;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_current_price"},
                 {"trading_pair", tradingPair}
             };
             
             // Add exchange ID if provided
             if (exchangeId) {
                 request["exchange_id"] = *exchangeId;
             }
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["price"].get<double>();
             } else {
                 LOG_ERROR("Failed to get current price: {}", responseJson["error"].get<std::string>());
                 return 0.0;
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get current price: {}", e.what());
             return 0.0;
         }
     }
     
     std::vector<std::pair<Timestamp, double>> getHistoricalPrices(
         const TradingPair& tradingPair,
         const std::string& interval,
         int count,
         const std::optional<ExchangeId>& exchangeId) override {
         
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return {};
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_historical_prices"},
                 {"trading_pair", tradingPair},
                 {"interval", interval},
                 {"count", count}
             };
             
             // Add exchange ID if provided
             if (exchangeId) {
                 request["exchange_id"] = *exchangeId;
             }
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 std::vector<std::pair<Timestamp, double>> prices;
                 for (const auto& price : responseJson["prices"]) {
                     prices.emplace_back(price["timestamp"].get<Timestamp>(), price["price"].get<double>());
                 }
                 return prices;
             } else {
                 LOG_ERROR("Failed to get historical prices: {}", responseJson["error"].get<std::string>());
                 return {};
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get historical prices: {}", e.what());
             return {};
         }
     }
     
     double getVolatility(const TradingPair& tradingPair, const std::string& timeframe, const std::optional<ExchangeId>& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return 0.0;
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_volatility"},
                 {"trading_pair", tradingPair},
                 {"timeframe", timeframe}
             };
             
             // Add exchange ID if provided
             if (exchangeId) {
                 request["exchange_id"] = *exchangeId;
             }
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 return responseJson["volatility"].get<double>();
             } else {
                 LOG_ERROR("Failed to get volatility: {}", responseJson["error"].get<std::string>());
                 return 0.0;
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get volatility: {}", e.what());
             return 0.0;
         }
     }
     
     std::vector<OrderRequest> getOrderHistory(const ExchangeId& exchangeId, const TradingPair& tradingPair, int limit) override {
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return {};
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_order_history"},
                 {"exchange_id", exchangeId},
                 {"trading_pair", tradingPair},
                 {"limit", limit}
             };
             
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
                 LOG_ERROR("Failed to get order history: {}", responseJson["error"].get<std::string>());
                 return {};
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get order history: {}", e.what());
             return {};
         }
     }
     
     std::map<std::string, double> getExchangeBalances(const ExchangeId& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return {};
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_exchange_balances"},
                 {"exchange_id", exchangeId}
             };
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 std::map<std::string, double> balances;
                 for (auto it = responseJson["balances"].begin(); it != responseJson["balances"].end(); ++it) {
                     balances[it.key()] = it.value().get<double>();
                 }
                 return balances;
             } else {
                 LOG_ERROR("Failed to get exchange balances: {}", responseJson["error"].get<std::string>());
                 return {};
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get exchange balances: {}", e.what());
             return {};
         }
     }
     
     void registerMarketDataCallback(const std::function<void(const MarketData&)>& callback) override {
         std::lock_guard<std::mutex> lock(mutex_);
         marketDataCallback_ = callback;
     }
     
     bool subscribeMarketData(const TradingPair& tradingPair, const std::optional<ExchangeId>& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return false;
         }
         
         try {
             // Create subscription key
             std::string subscriptionKey = generateSubscriptionKey(tradingPair, exchangeId);
             
             // Check if already subscribed
             if (activeSubscriptions_[subscriptionKey]) {
                 return true;
             }
             
             // Create message
             nlohmann::json request = {
                 {"action", "subscribe_market_data"},
                 {"trading_pair", tradingPair}
             };
             
             // Add exchange ID if provided
             if (exchangeId) {
                 request["exchange_id"] = *exchangeId;
             }
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 activeSubscriptions_[subscriptionKey] = true;
                 return true;
             } else {
                 LOG_ERROR("Failed to subscribe to market data: {}", responseJson["error"].get<std::string>());
                 return false;
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to subscribe to market data: {}", e.what());
             return false;
         }
     }
     
     bool unsubscribeMarketData(const TradingPair& tradingPair, const std::optional<ExchangeId>& exchangeId) override {
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return false;
         }
         
         try {
             // Create subscription key
             std::string subscriptionKey = generateSubscriptionKey(tradingPair, exchangeId);
             
             // Check if not subscribed
             if (!activeSubscriptions_[subscriptionKey]) {
                 return true;
             }
             
             // Create message
             nlohmann::json request = {
                 {"action", "unsubscribe_market_data"},
                 {"trading_pair", tradingPair}
             };
             
             // Add exchange ID if provided
             if (exchangeId) {
                 request["exchange_id"] = *exchangeId;
             }
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 activeSubscriptions_[subscriptionKey] = false;
                 return true;
             } else {
                 LOG_ERROR("Failed to unsubscribe from market data: {}", responseJson["error"].get<std::string>());
                 return false;
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to unsubscribe from market data: {}", e.what());
             return false;
         }
     }
     
     std::map<std::string, double> getTradingFees(const ExchangeId& exchangeId, const std::optional<TradingPair>& tradingPair) override {
         if (!initialized_) {
             LOG_ERROR("Data interface not initialized");
             return {};
         }
         
         try {
             // Create message
             nlohmann::json request = {
                 {"action", "get_trading_fees"},
                 {"exchange_id", exchangeId}
             };
             
             // Add trading pair if provided
             if (tradingPair) {
                 request["trading_pair"] = *tradingPair;
             }
             
             // Send request
             std::string response = zmqClient_->sendAndReceive(request.dump());
             
             // Parse response
             auto responseJson = nlohmann::json::parse(response);
             if (responseJson["success"].get<bool>()) {
                 std::map<std::string, double> fees;
                 for (auto it = responseJson["fees"].begin(); it != responseJson["fees"].end(); ++it) {
                     fees[it.key()] = it.value().get<double>();
                 }
                 return fees;
             } else {
                 LOG_ERROR("Failed to get trading fees: {}", responseJson["error"].get<std::string>());
                 return {};
             }
         } catch (const std::exception& e) {
             LOG_ERROR("Failed to get trading fees: {}", e.what());
             return {};
         }
     }
 };
 
 // Factory function to create DataInterface
 DataInterface::DataInterface() {
     // Base constructor, nothing to do here
 }
 
 // Global factory function to create the concrete implementation
 std::unique_ptr<DataInterface> createDataInterface() {
     return std::make_unique<DataInterfaceImpl>();
 }
 
 } // namespace risk
} // namespace trading