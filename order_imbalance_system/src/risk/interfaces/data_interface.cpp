/**
 * Implementation of the interface for accessing market data
 *
 * File: data_interface.cpp
 * Created: 2025-04-29
 */

#include "interfaces/data_interface.hpp"
#include <common/zeromq_client.hpp>
#include <common/logger.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <functional>
#include <iostream>
namespace trading {
namespace risk {
    
    
// Helper to log with timestamp (temporary until we integrate with logging system)
void logWithTimestamp(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S") << "] ";
    ss << "[" << level << "] " << message;
    std::cout << ss.str() << std::endl;
}

// Implementation of the DataInterface that uses ZeroMQ for communication
class DataInterfaceImpl : public DataInterface {
private:
    // Configuration
    std::string requestEndpoint_;
    std::string publishEndpoint_;
    std::string marketDataTopic_;
    
    // ZeroMQ clients
    std::unique_ptr<common::ZeroMQClient> requestClient_;
    std::unique_ptr<common::ZeroMQClient> subscriptionClient_;
    
    // Callback for market data updates
    std::function<void(const MarketData&)> marketDataCallback_;
    
    // Mutex for thread safety
    std::mutex mutex_;
    
    // Flag to indicate if initialization was successful
    bool initialized_ = false;
    
    // Active subscriptions
    std::map<std::string, bool> activeSubscriptions_;

    // Generate a subscription key
    std::string generateSubscriptionKey(const std::string& tradingPair, const std::optional<std::string>& exchangeId) {
        std::string key = tradingPair;
        if (exchangeId) {
            key += ":" + *exchangeId;
        }
        return key;
    }

    // Convert OrderRequest to JSON
    nlohmann::json orderRequestToJson(const OrderRequest& request) {
        nlohmann::json json;
        json["order_id"] = request.orderId;
        json["client_order_id"] = request.clientOrderId;
        json["exchangeId"] = request.exchangeId;
        json["tradingPair"] = request.tradingPair;
        json["side"] = request.side == OrderSide::BUY ? "buy" : "sell";
        json["type"] = request.type;
        json["quantity"] = request.quantity;
        json["price"] = request.price;
        json["time_in_force"] = request.timeInForce;
        json["strategy_id"] = request.strategyId;
        json["signal_id"] = request.signalId;
        json["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            request.timestamp.time_since_epoch()).count();
        
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
        request.tradingPair = json["tradingPair"];
        request.side = json["side"] == "buy" ? OrderSide::BUY : OrderSide::SELL;
        request.type = json["type"];
        request.quantity = json["quantity"];
        request.price = json["price"];
        request.timeInForce = json["time_in_force"];
        request.strategyId = json["strategy_id"];
        request.signalId = json["signal_id"];
        
        // Parse timestamp
        int64_t timestampMs = json["timestamp"];
        request.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestampMs));
        
        // Parse optional parameters if provided
        if (json.contains("params") && json["params"].is_object()) {
            for (auto it = json["params"].begin(); it != json["params"].end(); ++it) {
                request.params[it.key()] = it.value();
            }
        }
        
        return request;
    }
    
    // Parse MarketData from JSON
    MarketData jsonToMarketData(const nlohmann::json& json) {
        MarketData marketData;
        marketData.tradingPair = json["tradingPair"];
        marketData.exchangeId = json["exchangeId"];
        
        // Parse timestamp
        int64_t timestampMs = json["timestamp"];
        marketData.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestampMs));
        
        marketData.price = json["price"];
        marketData.volume = json["volume"];
        marketData.bid = json["bid"];
        marketData.ask = json["ask"];
        marketData.lastTradePrice = json["last_trade_price"];
        marketData.lastTradeQuantity = json["last_trade_quantity"];
        
        // Parse optional fields
        if (json.contains("open_24h")) marketData.open24h = json["open_24h"];
        if (json.contains("high_24h")) marketData.high24h = json["high_24h"];
        if (json.contains("low_24h")) marketData.low24h = json["low_24h"];
        if (json.contains("volume_quote_24h")) marketData.volumeQuote24h = json["volume_quote_24h"];
        
        // Parse any extra fields
        if (json.contains("extra") && json["extra"].is_object()) {
            for (auto it = json["extra"].begin(); it != json["extra"].end(); ++it) {
                if (it.value().is_number()) {
                    marketData.extra[it.key()] = it.value().get<double>();
                }
            }
        }
        
        return marketData;
    }
    
    // Handle received market data messages
    void handleMarketDataMessage(const std::string& topic, const std::string& message) {
        try {
            auto json = nlohmann::json::parse(message);
            auto marketData = jsonToMarketData(json);
            
            // Call the callback if registered
            if (marketDataCallback_) {
                marketDataCallback_(marketData);
            }
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Error processing market data: " + std::string(e.what()));
        }
    }

    // Handle error messages
    void handleErrorMessage(const std::string& context, const std::string& error) {
        logWithTimestamp("ERROR", "ZeroMQ error in " + context + ": " + error);
    }

public:
    DataInterfaceImpl() : DataInterface() {
        // Load configuration
        // In a real implementation, this would be loaded from a config file
        requestEndpoint_ = "tcp://localhost:5557";
        publishEndpoint_ = "tcp://localhost:5558";
        marketDataTopic_ = "market_data";
    }
    
    ~DataInterfaceImpl() override {
        // Clean shutdown of ZeroMQ clients
        if (subscriptionClient_) {
            subscriptionClient_->stop();
        }
        if (requestClient_) {
            requestClient_->stop();
        }
    }
    
    bool initialize() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) {
            return true;
        }
        
        try {
            // Configure ZeroMQ clients
            common::ZeroMQConfig requestConfig;
            requestConfig.publisherEndpoint = requestEndpoint_;
            
            common::ZeroMQConfig subscriptionConfig;
            subscriptionConfig.subscriberEndpoint = publishEndpoint_;
            subscriptionConfig.reconnectInterval = std::chrono::milliseconds(1000);
            subscriptionConfig.maxReconnectAttempts = 5;
            
            // Initialize request client
            requestClient_ = std::make_unique<common::ZeroMQClient>(requestConfig);
            
            // Initialize subscription client
            subscriptionClient_ = std::make_unique<common::ZeroMQClient>(subscriptionConfig);
            
            // Set message handler for subscription client
            subscriptionClient_->setMessageHandler(
                std::bind(&DataInterfaceImpl::handleMarketDataMessage, this, 
                          std::placeholders::_1, std::placeholders::_2));
            
            // Set error handler for both clients
            subscriptionClient_->setErrorHandler(
                std::bind(&DataInterfaceImpl::handleErrorMessage, this,
                          std::placeholders::_1, std::placeholders::_2));
            requestClient_->setErrorHandler(
                std::bind(&DataInterfaceImpl::handleErrorMessage, this,
                          std::placeholders::_1, std::placeholders::_2));
            
            // Start the clients
            if (!requestClient_->start()) {
                logWithTimestamp("ERROR", "Failed to start request client");
                return false;
            }
            
            if (!subscriptionClient_->start()) {
                logWithTimestamp("ERROR", "Failed to start subscription client");
                return false;
            }
            
            // Subscribe to the market data topic
            if (!subscriptionClient_->subscribe(marketDataTopic_)) {
                logWithTimestamp("ERROR", "Failed to subscribe to market data topic");
                return false;
            }
            
            initialized_ = true;
            logWithTimestamp("INFO", "Data interface initialized successfully");
            return true;
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to initialize data interface: " + std::string(e.what()));
            return false;
        }
    }
    
    MarketData getMarketData(const std::string& tradingPair, const std::optional<std::string>& exchangeId) override {
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
            return {};
        }
        
        try {
            // Create message
            nlohmann::json request = {
                {"action", "get_market_data"},
                {"tradingPair", tradingPair}
            };
            
            // Add exchange ID if provided
            if (exchangeId) {
                request["exchangeId"] = *exchangeId;
            }
            
            // Send request and receive response
            if (!requestClient_->publish("request", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send market data request");
                return {};
            }
            
            // In a real implementation, we would wait for the response with a timeout
            // For now, we'll just return an empty object
            // The actual implementation would use a request-response pattern with correlation IDs
            
            return {};
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to get market data: " + std::string(e.what()));
            return {};
        }
    }
    
    double getCurrentPrice(const std::string& tradingPair, const std::optional<std::string>& exchangeId) override {
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
            return 0.0;
        }
        
        try {
            // Create message
            nlohmann::json request = {
                {"action", "get_current_price"},
                {"tradingPair", tradingPair}
            };
            
            // Add exchange ID if provided
            if (exchangeId) {
                request["exchangeId"] = *exchangeId;
            }
            
            // Send request
            if (!requestClient_->publish("request", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send current price request");
                return 0.0;
            }
            
            // In a real implementation, we would wait for the response with a timeout
            // For now, we'll just return 0.0
            
            return 0.0;
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to get current price: " + std::string(e.what()));
            return 0.0;
        }
    }
    
    std::vector<std::pair<Timestamp, double>> getHistoricalPrices(
        const std::string& tradingPair,
        const std::string& interval,
        int count,
        const std::optional<std::string>& exchangeId) override {
        
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
            return {};
        }
        
        try {
            // Create message
            nlohmann::json request = {
                {"action", "get_historical_prices"},
                {"tradingPair", tradingPair},
                {"interval", interval},
                {"count", count}
            };
            
            // Add exchange ID if provided
            if (exchangeId) {
                request["exchangeId"] = *exchangeId;
            }
            
            // Send request
            if (!requestClient_->publish("request", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send historical prices request");
                return {};
            }
            
            // In a real implementation, we would wait for the response with a timeout
            // For now, we'll just return an empty vector
            
            return {};
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to get historical prices: " + std::string(e.what()));
            return {};
        }
    }
    
    double getVolatility(const std::string& tradingPair, const std::string& timeframe, const std::optional<std::string>& exchangeId) override {
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
            return 0.0;
        }
        
        try {
            // Create message
            nlohmann::json request = {
                {"action", "get_volatility"},
                {"tradingPair", tradingPair},
                {"timeframe", timeframe}
            };
            
            // Add exchange ID if provided
            if (exchangeId) {
                request["exchangeId"] = *exchangeId;
            }
            
            // Send request
            if (!requestClient_->publish("request", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send volatility request");
                return 0.0;
            }
            
            // In a real implementation, we would wait for the response with a timeout
            // For now, we'll just return 0.0
            
            return 0.0;
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to get volatility: " + std::string(e.what()));
            return 0.0;
        }
    }
    
    std::vector<OrderRequest> getOrderHistory(const std::string& exchangeId, const std::string& tradingPair, int limit) override {
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
            return {};
        }
        
        try {
            // Create message
            nlohmann::json request = {
                {"action", "get_order_history"},
                {"exchangeId", exchangeId},
                {"tradingPair", tradingPair},
                {"limit", limit}
            };
            
            // Send request
            if (!requestClient_->publish("request", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send order history request");
                return {};
            }
            
            // In a real implementation, we would wait for the response with a timeout
            // For now, we'll just return an empty vector
            
            return {};
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to get order history: " + std::string(e.what()));
            return {};
        }
    }
    
    std::map<std::string, double> getExchangeBalances(const std::string& exchangeId) override {
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
            return {};
        }
        
        try {
            // Create message
            nlohmann::json request = {
                {"action", "get_exchange_balances"},
                {"exchangeId", exchangeId}
            };
            
            // Send request
            if (!requestClient_->publish("request", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send exchange balances request");
                return {};
            }
            
            // In a real implementation, we would wait for the response with a timeout
            // For now, we'll just return an empty map
            
            return {};
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to get exchange balances: " + std::string(e.what()));
            return {};
        }
    }
    
    void registerMarketDataCallback(const std::function<void(const MarketData&)>& callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        marketDataCallback_ = callback;
    }
    
    bool subscribeMarketData(const std::string& tradingPair, const std::optional<std::string>& exchangeId) override {
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
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
                {"tradingPair", tradingPair}
            };
            
            // Add exchange ID if provided
            if (exchangeId) {
                request["exchangeId"] = *exchangeId;
            }
            
            // Send request
            if (!requestClient_->publish("subscription", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send market data subscription request");
                return false;
            }
            
            // Mark as subscribed
            activeSubscriptions_[subscriptionKey] = true;
            
            return true;
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to subscribe to market data: " + std::string(e.what()));
            return false;
        }
    }
    
    bool unsubscribeMarketData(const std::string& tradingPair, const std::optional<std::string>& exchangeId) override {
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
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
                {"tradingPair", tradingPair}
            };
            
            // Add exchange ID if provided
            if (exchangeId) {
                request["exchangeId"] = *exchangeId;
            }
            
            // Send request
            if (!requestClient_->publish("subscription", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send market data unsubscription request");
                return false;
            }
            
            // Mark as unsubscribed
            activeSubscriptions_[subscriptionKey] = false;
            
            return true;
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to unsubscribe from market data: " + std::string(e.what()));
            return false;
        }
    }
    
    std::map<std::string, double> getTradingFees(const std::string& exchangeId, const std::optional<std::string>& tradingPair) override {
        if (!initialized_) {
            logWithTimestamp("ERROR", "Data interface not initialized");
            return {};
        }
        
        try {
            // Create message
            nlohmann::json request = {
                {"action", "get_trading_fees"},
                {"exchangeId", exchangeId}
            };
            
            // Add trading pair if provided
            if (tradingPair) {
                request["tradingPair"] = *tradingPair;
            }
            
            // Send request
            if (!requestClient_->publish("request", request.dump())) {
                logWithTimestamp("ERROR", "Failed to send trading fees request");
                return {};
            }
            
            // In a real implementation, we would wait for the response with a timeout
            // For now, we'll just return an empty map
            
            return {};
        } catch (const std::exception& e) {
            logWithTimestamp("ERROR", "Failed to get trading fees: " + std::string(e.what()));
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