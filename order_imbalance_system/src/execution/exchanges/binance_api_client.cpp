// exchanges/binance_api_client.cpp
#include "execution/exchanges/binance_api_client.hpp"
#include "execution/models/order.hpp"
#include "execution/models/execution_report.hpp"
#include <curl/curl.h>
#include <json/json.h>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <chrono>
#include <thread>
#include <future>
#include <stdexcept>

namespace trading {
namespace execution {

// Private implementation class
struct BinanceApiClient::BinanceImpl {
    std::string apiKey;
    std::string apiSecret;
    std::string baseUrl = "https://api.binance.com";
    bool connected = false;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl;
    
    // Constructor
    BinanceImpl(const ExchangeCredentials& credentials) 
        : apiKey(credentials.apiKey), 
          apiSecret(credentials.apiSecret),
          curl(curl_easy_init(), curl_easy_cleanup) {
        if (!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }
    
    // CURL callback for writing response
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
};

BinanceApiClient::BinanceApiClient(const ExchangeCredentials& credentials, 
                                 std::shared_ptr<RateLimitController> rateLimitController)
    : pImpl_(std::make_unique<BinanceImpl>(credentials)),
      rateLimitController_(std::move(rateLimitController)) {
    // Initialize curl globally if not already done
    static std::once_flag flag;
    std::call_once(flag, []() { curl_global_init(CURL_GLOBAL_ALL); });
}

BinanceApiClient::~BinanceApiClient() = default;

void BinanceApiClient::connect() {
    if (pImpl_->connected) return;
    
    try {
        // Test connection by getting exchange info
        std::map<std::string, std::string> params;
        std::string response = makeApiRequest("/api/v3/exchangeInfo", "GET", params, false);
        
        // Parse response to verify it's valid
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(response, root)) {
            throw std::runtime_error("Failed to parse Binance exchange info");
        }
        
        pImpl_->connected = true;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to connect to Binance: ") + e.what());
    }
}

void BinanceApiClient::disconnect() {
    pImpl_->connected = false;
}

bool BinanceApiClient::isConnected() const {
    return pImpl_->connected;
}

std::string BinanceApiClient::getExchangeName() const {
    return "BINANCE";
}

std::vector<std::string> BinanceApiClient::getSupportedSymbols() const {
    if (!isConnected()) {
        throw std::runtime_error("Not connected to Binance");
    }
    
    // Get exchange info to retrieve supported symbols
    std::map<std::string, std::string> params;
    std::string response = makeApiRequest("/api/v3/exchangeInfo", "GET", params, false);
    
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse Binance exchange info");
    }
    
    std::vector<std::string> symbols;
    const Json::Value& symbolsJson = root["symbols"];
    for (const auto& symbolJson : symbolsJson) {
        if (symbolJson["status"].asString() == "TRADING") {
            symbols.push_back(symbolJson["symbol"].asString());
        }
    }
    
    return symbols;
}

std::future<ExecutionReport> BinanceApiClient::placeOrder(const Order& order) {
    return std::async(std::launch::async, [this, order]() {
        if (!isConnected()) {
            connect();
        }
        
        // Wait for rate limit
        rateLimitController_->waitForPermission(ExchangeRequestType::ORDER_PLACEMENT);
        
        // Prepare parameters
        std::map<std::string, std::string> params;
        params["symbol"] = order.getTradingPair();
        params["side"] = translateOrderSide(order.getSide());
        params["type"] = translateOrderType(order.getType());
        params["quantity"] = std::to_string(order.getQuantity());
        
        if (order.getType() != OrderType::MARKET) {
            params["price"] = std::to_string(order.getPrice());
            params["timeInForce"] = translateTimeInForce(order.getTimeInForce());
        }
        
        if (!order.getClientOrderId().empty()) {
            params["newClientOrderId"] = order.getClientOrderId();
        }
        
        // Add extra parameters if present
        for (const auto& [key, value] : order.getExtraParams()) {
            params[key] = value;
        }
        
        // Add timestamp
        params["timestamp"] = std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        
        // Send request
        std::string response = makeApiRequest("/api/v3/order", "POST", params, true);
        
        // Parse response
        return parseOrderResponse(response);
    });
}

std::future<ExecutionReport> BinanceApiClient::cancelOrder(const std::string& orderId, const std::string& symbol) {
    return std::async(std::launch::async, [this, orderId, symbol]() {
        if (!isConnected()) {
            connect();
        }
        
        // Wait for rate limit
        rateLimitController_->waitForPermission(ExchangeRequestType::ORDER_CANCELLATION);
        
        // Prepare parameters
        std::map<std::string, std::string> params;
        params["symbol"] = symbol;
        
        // Check if orderId is a client order ID or exchange order ID
        if (orderId.find("CLIENT:") == 0) {
            // It's a client order ID
            params["origClientOrderId"] = orderId.substr(7);
        } else {
            // It's an exchange order ID
            params["orderId"] = orderId;
        }
        
        // Add timestamp
        params["timestamp"] = std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        
        // Send request
        std::string response = makeApiRequest("/api/v3/order", "DELETE", params, true);
        
        // Parse response
        return parseOrderResponse(response);
    });
}

std::future<ExecutionReport> BinanceApiClient::getOrderStatus(const std::string& orderId, const std::string& symbol) {
    return std::async(std::launch::async, [this, orderId, symbol]() {
        if (!isConnected()) {
            connect();
        }
        
        // Wait for rate limit
        rateLimitController_->waitForPermission(ExchangeRequestType::ORDER_STATUS);
        
        // Prepare parameters
        std::map<std::string, std::string> params;
        params["symbol"] = symbol;
        
        // Check if orderId is a client order ID or exchange order ID
        if (orderId.find("CLIENT:") == 0) {
            // It's a client order ID
            params["origClientOrderId"] = orderId.substr(7);
        } else {
            // It's an exchange order ID
            params["orderId"] = orderId;
        }
        
        // Add timestamp
        params["timestamp"] = std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        
        // Send request
        std::string response = makeApiRequest("/api/v3/order", "GET", params, true);
        
        // Parse response
        return parseOrderResponse(response);
    });
}

std::future<std::map<std::string, double>> BinanceApiClient::getAccountBalances() {
    return std::async(std::launch::async, [this]() {
        if (!isConnected()) {
            connect();
        }
        
        // Wait for rate limit
        rateLimitController_->waitForPermission(ExchangeRequestType::ACCOUNT_INFO);
        
        // Prepare parameters
        std::map<std::string, std::string> params;
        params["timestamp"] = std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        
        // Send request
        std::string response = makeApiRequest("/api/v3/account", "GET", params, true);
        
        // Parse response
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(response, root)) {
            throw std::runtime_error("Failed to parse Binance account information");
        }
        
        std::map<std::string, double> balances;
        const Json::Value& balancesJson = root["balances"];
        for (const auto& balanceJson : balancesJson) {
            std::string asset = balanceJson["asset"].asString();
            double free = std::stod(balanceJson["free"].asString());
            double locked = std::stod(balanceJson["locked"].asString());
            
            // Sum free and locked amounts
            balances[asset] = free + locked;
        }
        
        return balances;
    });
}

std::future<double> BinanceApiClient::getCurrentPrice(const std::string& symbol) {
    return std::async(std::launch::async, [this, symbol]() {
        if (!isConnected()) {
            connect();
        }
        
        // Wait for rate limit
        rateLimitController_->waitForPermission(ExchangeRequestType::MARKET_DATA);
        
        // Prepare parameters
        std::map<std::string, std::string> params;
        params["symbol"] = symbol;
        
        // Send request
        std::string response = makeApiRequest("/api/v3/ticker/price", "GET", params, false);
        
        // Parse response
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(response, root)) {
            throw std::runtime_error("Failed to parse Binance price information");
        }
        
        return std::stod(root["price"].asString());
    });
}

int BinanceApiClient::getRemainingRequests(ExchangeRequestType requestType) const {
    return rateLimitController_->getRemainingRequests(requestType);
}

std::chrono::milliseconds BinanceApiClient::getResetTimeForRequestType(ExchangeRequestType requestType) const {
    return rateLimitController_->getResetTime(requestType);
}

std::string BinanceApiClient::makeApiRequest(const std::string& endpoint, const std::string& method, 
                                           const std::map<std::string, std::string>& params, bool isPrivate) {
    std::string url = pImpl_->baseUrl + endpoint;
    std::string queryString;
    
    // Build query string
    for (const auto& [key, value] : params) {
        if (!queryString.empty()) {
            queryString += "&";
        }
        queryString += key + "=" + value;
    }
    
    // For GET requests, append query string to URL
    if (method == "GET" && !queryString.empty()) {
        url += "?" + queryString;
    }
    
    // Set up CURL
    curl_easy_reset(pImpl_->curl.get());
    curl_easy_setopt(pImpl_->curl.get(), CURLOPT_URL, url.c_str());
    
    // Set request method
    if (method == "POST") {
        curl_easy_setopt(pImpl_->curl.get(), CURLOPT_POST, 1L);
        if (!queryString.empty()) {
            curl_easy_setopt(pImpl_->curl.get(), CURLOPT_POSTFIELDS, queryString.c_str());
        }
    } else if (method == "DELETE") {
        curl_easy_setopt(pImpl_->curl.get(), CURLOPT_CUSTOMREQUEST, "DELETE");
        if (!queryString.empty() && url.find('?') == std::string::npos) {
            url += "?" + queryString;
            curl_easy_setopt(pImpl_->curl.get(), CURLOPT_URL, url.c_str());
        }
    }
    
    // Set response callback
    std::string responseBuffer;
    curl_easy_setopt(pImpl_->curl.get(), CURLOPT_WRITEFUNCTION, BinanceImpl::WriteCallback);
    curl_easy_setopt(pImpl_->curl.get(), CURLOPT_WRITEDATA, &responseBuffer);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    
    if (isPrivate) {
        // Add API key header
        std::string apiKeyHeader = "X-MBX-APIKEY: " + pImpl_->apiKey;
        headers = curl_slist_append(headers, apiKeyHeader.c_str());
        
        // Add signature
        std::string signature = generateSignature(queryString);
        if (method == "GET" || method == "DELETE") {
            if (url.find('?') != std::string::npos) {
                url += "&signature=" + signature;
            } else {
                url += "?signature=" + signature;
            }
            curl_easy_setopt(pImpl_->curl.get(), CURLOPT_URL, url.c_str());
        } else {
            // For POST, add to query string
            if (!queryString.empty()) {
                queryString += "&signature=" + signature;
            } else {
                queryString = "signature=" + signature;
            }
            curl_easy_setopt(pImpl_->curl.get(), CURLOPT_POSTFIELDS, queryString.c_str());
        }
    }
    
    curl_easy_setopt(pImpl_->curl.get(), CURLOPT_HTTPHEADER, headers);
    
    // Execute request
    CURLcode res = curl_easy_perform(pImpl_->curl.get());
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
    }
    
    // Check HTTP response code
    long httpCode = 0;
    curl_easy_getinfo(pImpl_->curl.get(), CURLINFO_RESPONSE_CODE, &httpCode);
    
    if (httpCode >= 400) {
        throw std::runtime_error("HTTP error " + std::to_string(httpCode) + ": " + responseBuffer);
    }
    
    return responseBuffer;
}

std::string BinanceApiClient::generateSignature(const std::string& queryString) const {
    // Create HMAC-SHA256 signature
    unsigned char* digest = HMAC(EVP_sha256(), 
                              pImpl_->apiSecret.c_str(), 
                              pImpl_->apiSecret.size(),
                              reinterpret_cast<const unsigned char*>(queryString.c_str()),
                              queryString.size(),
                              nullptr, nullptr);
    
    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    
    return ss.str();
}

std::string BinanceApiClient::translateOrderType(OrderType type) const {
    switch (type) {
        case OrderType::MARKET: return "MARKET";
        case OrderType::LIMIT: return "LIMIT";
        case OrderType::STOP: return "STOP_LOSS";
        case OrderType::STOP_LIMIT: return "STOP_LOSS_LIMIT";
        case OrderType::TRAILING_STOP: return "TRAILING_STOP_MARKET";
        case OrderType::FILL_OR_KILL: return "LIMIT";  // With timeInForce = FOK
        case OrderType::IMMEDIATE_OR_CANCEL: return "LIMIT";  // With timeInForce = IOC
        case OrderType::POST_ONLY: return "LIMIT";  // Special flag
        case OrderType::ICEBERG: return "LIMIT";  // With icebergQty
        default: throw std::invalid_argument("Unsupported order type for Binance");
    }
}

std::string BinanceApiClient::translateTimeInForce(TimeInForce tif) const {
    switch (tif) {
        case TimeInForce::GTC: return "GTC";  // Good Till Canceled
        case TimeInForce::IOC: return "IOC";  // Immediate Or Cancel
        case TimeInForce::FOK: return "FOK";  // Fill Or Kill
        case TimeInForce::GTD: return "GTC";  // Binance doesn't directly support GTD
        default: return "GTC";  // Default to GTC
    }
}

std::string BinanceApiClient::translateOrderSide(OrderSide side) const {
    switch (side) {
        case OrderSide::BUY: return "BUY";
        case OrderSide::SELL: return "SELL";
        default: throw std::invalid_argument("Invalid order side");
    }
}

ExecutionReport BinanceApiClient::parseOrderResponse(const std::string& response) const {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse Binance order response");
    }
    
    ExecutionReport report;
    
    // Check for error response
    if (root.isMember("code") && root["code"].asInt() != 0) {
        report.setStatus(OrderState::REJECTED);
        report.setRejectReason(root["msg"].asString());
        return report;
    }
    
    // Parse successful response
    report.setExchangeId("BINANCE");
    report.setOrderId(root["clientOrderId"].asString());
    report.setExchangeOrderId(root["orderId"].asString());
    report.setSymbol(root["symbol"].asString());
    
    // Parse order side
    std::string side = root["side"].asString();
    report.setSide(side == "BUY" ? OrderSide::BUY : OrderSide::SELL);
    
    // Parse order type
    std::string type = root["type"].asString();
    if (type == "MARKET") {
        report.setType(OrderType::MARKET);
    } else if (type == "LIMIT") {
        report.setType(OrderType::LIMIT);
    } else if (type == "STOP_LOSS") {
        report.setType(OrderType::STOP);
    } else if (type == "STOP_LOSS_LIMIT") {
        report.setType(OrderType::STOP_LIMIT);
    } else {
        // Handle other types as needed
        report.setType(OrderType::LIMIT);
    }
    
    // Parse order status
    std::string status = root["status"].asString();
    if (status == "NEW" || status == "PARTIALLY_FILLED") {
        report.setStatus(status == "NEW" ? OrderState::OPEN : OrderState::PARTIALLY_FILLED);
    } else if (status == "FILLED") {
        report.setStatus(OrderState::FILLED);
    } else if (status == "CANCELED" || status == "PENDING_CANCEL" || status == "EXPIRED" || status == "REJECTED") {
        report.setStatus(
            status == "CANCELED" ? OrderState::CANCELED :
            status == "PENDING_CANCEL" ? OrderState::PENDING_CANCEL :
            status == "EXPIRED" ? OrderState::EXPIRED : OrderState::REJECTED
        );
    }
    
    // Parse quantities
    report.setQuantity(std::stod(root["origQty"].asString()));
    report.setFilledQuantity(std::stod(root["executedQty"].asString()));
    
    // Parse price if present
    if (root.isMember("price") && !root["price"].asString().empty()) {
        report.setPrice(std::stod(root["price"].asString()));
    }
    
    // Parse fills if present
    if (root.isMember("fills") && root["fills"].isArray()) {
        const Json::Value& fills = root["fills"];
        for (const auto& fill : fills) {
            Fill f;
            f.fillId = fill["tradeId"].asString();
            f.orderId = root["orderId"].asString();
            f.quantity = std::stod(fill["qty"].asString());
            f.price = std::stod(fill["price"].asString());
            f.fee = std::stod(fill["commission"].asString());
            f.feeCurrency = fill["commissionAsset"].asString();
            
            // Parse timestamp
            auto ms = std::chrono::milliseconds(std::stoll(fill["time"].asString()));
            f.timestamp = std::chrono::system_clock::time_point(ms);
            
            report.addFill(f);
        }
    }
    
    // Set timestamps
    if (root.isMember("time")) {
        auto ms = std::chrono::milliseconds(std::stoll(root["time"].asString()));
        report.setCreatedAt(std::chrono::system_clock::time_point(ms));
    }
    
    // Add any exchange-specific data
    if (root.isMember("icebergQty")) {
        report.setExtraData("icebergQty", root["icebergQty"].asString());
    }
    
    if (root.isMember("stopPrice")) {
        report.setExtraData("stopPrice", root["stopPrice"].asString());
    }
    
    return report;
}

} // namespace execution
} // namespace trading