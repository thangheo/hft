#include "../market_data_handler.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <prometheus/registry.h>
#include "mock_exchange_adapter.h"  // Assuming you have a mock adapter

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

// // Mock class for the exchange adapter
// class MockExchangeAdapter : public IExchangeAdapter {
// public:
//     MOCK_METHOD(bool, connect, (), (override));
//     MOCK_METHOD(void, disconnect, (), (override));
//     MOCK_METHOD(void, subscribe, (const std::string& symbol), (override));
//     MOCK_METHOD(void, unsubscribe, (const std::string& symbol), (override));
// };

// Test Fixture for MarketDataHandler
class MarketDataHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_ = std::make_shared<prometheus::Registry>();
        handler_ = std::make_unique<MarketDataHandler>(registry_);

        // Set up mock adapter
        mock_adapter_ = std::make_shared<MockExchangeAdapter>();
    }

    std::shared_ptr<prometheus::Registry> registry_;
    std::unique_ptr<MarketDataHandler> handler_;
    std::shared_ptr<MockExchangeAdapter> mock_adapter_;
};

// Test connection to an exchange
TEST_F(MarketDataHandlerTest, ConnectToExchange) {
    // Inject mock adapter into the handler
    handler_->exchange_adapters_["binance"] = mock_adapter_;
    EXPECT_CALL(*mock_adapter_, connect()).WillOnce(Return(true));

    EXPECT_TRUE(handler_->connect("binance"));
}

// Test disconnection from an exchange
TEST_F(MarketDataHandlerTest, DisconnectFromExchange) {
    handler_->exchange_adapters_["binance"] = mock_adapter_;
    EXPECT_CALL(*mock_adapter_, disconnect()).Times(1);

    handler_->disconnect("binance");
}

// Test subscribing to a symbol
TEST_F(MarketDataHandlerTest, SubscribeToSymbol) {
    handler_->exchange_adapters_["binance"] = mock_adapter_;
    EXPECT_CALL(*mock_adapter_, subscribe("BTCUSD")).Times(1);

    handler_->subscribe("BTCUSD");
}

// Test unsubscribing from a symbol
TEST_F(MarketDataHandlerTest, UnsubscribeFromSymbol) {
    handler_->exchange_adapters_["binance"] = mock_adapter_;
    EXPECT_CALL(*mock_adapter_, unsubscribe("BTCUSD")).Times(1);

    handler_->unsubscribe("BTCUSD");
}

// Test market data callback
TEST_F(MarketDataHandlerTest, MarketDataCallback) {
    bool callback_called = false;
    MarketData sample_data;
    handler_->setMarketDataCallback([&](const MarketData& data) {
        callback_called = true;
    });

    handler_->onExchangeData("binance", "sample data");

    EXPECT_TRUE(callback_called);
}
