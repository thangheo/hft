// tests/data_ingestion/test_exchange_manager.cpp
#include "data_ingestion/exchange_manager.hpp"
#include "data_ingestion/websocket/connection_pool.hpp"
#include "data_ingestion/websocket/client_base.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace trading::data_ingestion;
using namespace ::testing;

namespace {

// Mock WebSocket client for testing
class MockWebSocketClient : public WebSocketClientBase {
public:
    MockWebSocketClient(const std::string& host, const std::string& port)
        : WebSocketClientBase(host, port, false) {
    }
    
    MOCK_METHOD0(connect, bool());
    MOCK_CONST_METHOD0(isConnected, bool());
    
    // Mock subscription methods
    MOCK_METHOD1(subscribe, bool(const std::vector<std::string>&));
    
protected:
    void onOpen() override {}
    void onMessage(const std::string& message) override {}
    void onClose(int code, const std::string& reason) override {}
    void onError(const std::string& error) override {}
    void onReconnect() override {}
};

// Mock ConnectionPool for testing
class MockConnectionPool : public ConnectionPool {
public:
    MockConnectionPool(const std::string& pool_id)
        : ConnectionPool(pool_id, []() { return nullptr; }, 1, 1),
          healthy_(true) {
    }
    
    MOCK_METHOD0(initialize, void());
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD1(acquireConnection, std::shared_ptr<WebSocketClientBase>(int));
    MOCK_METHOD1(releaseConnection, void(std::shared_ptr<WebSocketClientBase>));
    
    // Override health check method
    bool isHealthy() const override {
        return healthy_;
    }
    
    // Control health state for tests
    void setHealthy(bool healthy) {
        healthy_ = healthy;
    }
    
    // Mock statistics methods
    MOCK_CONST_METHOD0(getActiveConnectionCount, int());
    MOCK_CONST_METHOD0(getAvailableConnectionCount, int());
    MOCK_CONST_METHOD0(getTotalConnectionCount, int());
    
private:
    bool healthy_;
};

// Mock OrderBookNormalizer for testing
class MockOrderBookNormalizer {
public:
    MOCK_METHOD1(processMarketData, void(const MarketDataUpdate&));
    MOCK_CONST_METHOD3(getOrderBook, OrderBookUpdate(const std::string&, const std::string&, int32_t));
};

// Callback converter for OrderBookNormalizer
void normalizedOrderBookCallback(const OrderBookUpdate& update, MockOrderBookNormalizer* normalizer) {
    // Nothing to do here - we mock the normalizer
}

} // anonymous namespace

// Test fixture
class ExchangeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_normalizer_ = std::make_shared<MockOrderBookNormalizer>();
        
        // Create callback that forwards to the mock
        auto callback = std::bind(normalizedOrderBookCallback, std::placeholders::_1, mock_normalizer_.get());
        
        // Create the exchange manager with a stubbed normalizer
        manager_ = std::make_unique<ExchangeManager>(callback);
        
        // Set up default behaviors for pools and clients
        SetUpMockPools();
        SetUpMockClients();
    }
    
    void TearDown() override {
        // Clean up
        manager_.reset();
        pool_factories_.clear();
        mock_pools_.clear();
        mock_clients_.clear();
        mock_normalizer_.reset();
    }
    
    // Create and register a mock pool for testing
    bool registerMockPool(const std::string& exchange_id) {
        // Create the mock pool
        auto mock_pool = std::make_shared<NiceMock<MockConnectionPool>>(exchange_id);
        mock_pools_[exchange_id] = mock_pool;
        
        // Create a factory that returns the mock pool
        ConnectionPoolFactory factory = [this, exchange_id, mock_pool]
            (MarketDataCallback callback) -> std::unique_ptr<ConnectionPool> {
                // Store the callback for testing
                market_data_callbacks_[exchange_id] = callback;
                
                // Return a unique_ptr to the mock pool (but keep a shared_ptr for test access)
                return std::unique_ptr<ConnectionPool>(
                    reinterpret_cast<ConnectionPool*>(new NiceMock<MockConnectionPool>(*mock_pool))
                );
            };
        
        // Store the factory for later reference
        pool_factories_[exchange_id] = factory;
        
        // Register with the exchange manager
        return manager_->registerExchange(exchange_id, factory, 1, 2);
    }
    
    // Set up default behaviors for mock pools
    void SetUpMockPools() {
        ON_CALL(mock_pool_template_, initialize())
            .WillByDefault(Return());
        
        ON_CALL(mock_pool_template_, shutdown())
            .WillByDefault(Return());
        
        ON_CALL(mock_pool_template_, getActiveConnectionCount())
            .WillByDefault(Return(0));
        
        ON_CALL(mock_pool_template_, getAvailableConnectionCount())
            .WillByDefault(Return(1));
        
        ON_CALL(mock_pool_template_, getTotalConnectionCount())
            .WillByDefault(Return(1));
    }
    
    // Set up default behaviors for mock clients
    void SetUpMockClients() {
        ON_CALL(mock_client_template_, connect())
            .WillByDefault(Return(true));
        
        ON_CALL(mock_client_template_, isConnected())
            .WillByDefault(Return(true));
        
        ON_CALL(mock_client_template_, subscribe(_))
            .WillByDefault(Return(true));
    }
    
    // Create a mock client for a given exchange
    std::shared_ptr<MockWebSocketClient> createMockClient(const std::string& exchange_id) {
        auto client = std::make_shared<NiceMock<MockWebSocketClient>>("test-host", "443");
        mock_clients_[exchange_id] = client;
        
        // Set up the pool to return this client
        if (mock_pools_.find(exchange_id) != mock_pools_.end()) {
            ON_CALL(*mock_pools_[exchange_id], acquireConnection(_))
                .WillByDefault(Return(client));
        }
        
        return client;
    }
    
    // Test data
    std::unique_ptr<ExchangeManager> manager_;
    std::shared_ptr<MockOrderBookNormalizer> mock_normalizer_;
    
    // Mock pools and factories for testing
    std::map<std::string, ConnectionPoolFactory> pool_factories_;
    std::map<std::string, std::shared_ptr<MockConnectionPool>> mock_pools_;
    std::map<std::string, MarketDataCallback> market_data_callbacks_;
    
    // Mock clients for testing
    std::map<std::string, std::shared_ptr<MockWebSocketClient>> mock_clients_;
    
    // Templates for setting up default behaviors
    NiceMock<MockConnectionPool> mock_pool_template_{"template"};
    NiceMock<MockWebSocketClient> mock_client_template_{"template-host", "443"};
};

// Test exchange registration
TEST_F(ExchangeManagerTest, ExchangeRegistration) {
    // Register a mock exchange
    EXPECT_TRUE(registerMockPool("binance"));
    
    // Try to register the same exchange again - should fail
    EXPECT_FALSE(registerMockPool("binance"));
    
    // Register another exchange
    EXPECT_TRUE(registerMockPool("coinbase"));
    
    // Check that we have the expected exchanges
    auto connected = manager_->getConnectedExchanges();
    EXPECT_EQ(2, connected.size());
    EXPECT_NE(std::find(connected.begin(), connected.end(), "binance"), connected.end());
    EXPECT_NE(std::find(connected.begin(), connected.end(), "coinbase"), connected.end());
}

// Test exchange removal
TEST_F(ExchangeManagerTest, ExchangeRemoval) {
    // Register exchanges
    ASSERT_TRUE(registerMockPool("binance"));
    ASSERT_TRUE(registerMockPool("coinbase"));
    
    // Remove one exchange
    EXPECT_TRUE(manager_->removeExchange("binance"));
    
    // Try to remove a non-existent exchange
    EXPECT_FALSE(manager_->removeExchange("nonexistent"));
    
    // Check that we have the expected exchanges
    auto connected = manager_->getConnectedExchanges();
    EXPECT_EQ(1, connected.size());
    EXPECT_EQ(std::find(connected.begin(), connected.end(), "binance"), connected.end());
    EXPECT_NE(std::find(connected.begin(), connected.end(), "coinbase"), connected.end());
}

// Test subscription
TEST_F(ExchangeManagerTest, Subscribe) {
    // Register an exchange
    ASSERT_TRUE(registerMockPool("binance"));
    
    // Create a mock client
    auto client = createMockClient("binance");
    
    // Define test data
    std::vector<std::string> trading_pairs = {"BTC/USDT", "ETH/USDT"};
    std::vector<DataType> data_types = {DataType::ORDER_BOOK, DataType::TRADES};
    
    // Expect the client to be acquired and subscription to be called
    EXPECT_CALL(*mock_pools_["binance"], acquireConnection(_))
        .WillOnce(Return(client));
    
    EXPECT_CALL(*mock_pools_["binance"], releaseConnection(_))
        .Times(1);
    
    EXPECT_CALL(*client, subscribe(_))
        .WillOnce(Return(true));
    
    // Subscribe
    EXPECT_TRUE(manager_->subscribe("binance", trading_pairs, data_types, 10));
    
    // Try to subscribe to a non-existent exchange
    EXPECT_FALSE(manager_->subscribe("nonexistent", trading_pairs, data_types, 10));
}

// Test subscribe all
TEST_F(ExchangeManagerTest, SubscribeAll) {
    // Register exchanges
    ASSERT_TRUE(registerMockPool("binance"));
    ASSERT_TRUE(registerMockPool("coinbase"));
    
    // Create mock clients
    auto binance_client = createMockClient("binance");
    auto coinbase_client = createMockClient("coinbase");
    
    // Define test data
    std::vector<std::string> trading_pairs = {"BTC/USDT", "ETH/USDT"};
    std::vector<DataType> data_types = {DataType::ORDER_BOOK};
    
    // Expect clients to be acquired and subscriptions to be called
    EXPECT_CALL(*mock_pools_["binance"], acquireConnection(_))
        .WillOnce(Return(binance_client));
    
    EXPECT_CALL(*mock_pools_["coinbase"], acquireConnection(_))
        .WillOnce(Return(coinbase_client));
    
    EXPECT_CALL(*mock_pools_["binance"], releaseConnection(_))
        .Times(1);
    
    EXPECT_CALL(*mock_pools_["coinbase"], releaseConnection(_))
        .Times(1);
    
    EXPECT_CALL(*binance_client, subscribe(_))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*coinbase_client, subscribe(_))
        .WillOnce(Return(true));
    
    // Subscribe to all exchanges
    manager_->subscribeAll(trading_pairs, data_types, 10);
}

// Test market data processing
TEST_F(ExchangeManagerTest, MarketDataProcessing) {
    // Register an exchange
    ASSERT_TRUE(registerMockPool("binance"));
    
    // Expect the normalizer to process market data
    EXPECT_CALL(*mock_normalizer_, processMarketData(_))
        .Times(1);
    
    // Create and send a market data update
    MarketDataUpdate update;
    update.exchange_id = "binance";
    update.trading_pair = "BTC/USDT";
    update.update_type = "orderbook";
    update.exchange_timestamp = 1625239046123;
    update.received_timestamp = 1625239046123;
    update.data = {'t', 'e', 's', 't'};
    
    // Send the update via the callback
    market_data_callbacks_["binance"](update);
}

// Test order book retrieval
TEST_F(ExchangeManagerTest, OrderBookRetrieval) {
    // Register an exchange
    ASSERT_TRUE(registerMockPool("binance"));
    
    // Set up expected response
    OrderBookUpdate expected_book;
    expected_book.exchange_id = "binance";
    expected_book.trading_pair = "BTC/USDT";
    
    // Expect the normalizer to be called
    EXPECT_CALL(*mock_normalizer_, getOrderBook("binance", "BTC/USDT", 10))
        .WillOnce(Return(expected_book));
    
    // Get the order book
    auto book = manager_->getOrderBook("binance", "BTC/USDT", 10);
    
    // Verify the result
    EXPECT_EQ("binance", book.exchange_id);
    EXPECT_EQ("BTC/USDT", book.trading_pair);
}

// Test connection monitoring
TEST_F(ExchangeManagerTest, ConnectionMonitoring) {
    // Register exchanges
    ASSERT_TRUE(registerMockPool("binance"));
    ASSERT_TRUE(registerMockPool("coinbase"));
    
    // Initially all exchanges should be connected
    EXPECT_EQ(2, manager_->getConnectedExchanges().size());
    
    // Simulate one exchange becoming unhealthy
    mock_pools_["binance"]->setHealthy(false);
    
    // Allow the monitoring thread some time to detect the change
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Give the monitor thread a chance to detect the change
    // (this assumes the monitor thread checks frequently enough)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // After the change, only the healthy exchange should be reported as connected
    auto connected = manager_->getConnectedExchanges();
    EXPECT_EQ(1, connected.size());
    EXPECT_EQ(std::find(connected.begin(), connected.end(), "binance"), connected.end());
    EXPECT_NE(std::find(connected.begin(), connected.end(), "coinbase"), connected.end());
}

// Test disconnect all
TEST_F(ExchangeManagerTest, DisconnectAll) {
    // Register exchanges
    ASSERT_TRUE(registerMockPool("binance"));
    ASSERT_TRUE(registerMockPool("coinbase"));
    
    // Expect shutdown to be called on all pools
    EXPECT_CALL(*mock_pools_["binance"], shutdown())
        .Times(1);
    
    EXPECT_CALL(*mock_pools_["coinbase"], shutdown())
        .Times(1);
    
    // Disconnect all
    manager_->disconnectAll();
    
    // All exchanges should be disconnected
    EXPECT_EQ(0, manager_->getConnectedExchanges().size());
}

// Main function required to run the tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
