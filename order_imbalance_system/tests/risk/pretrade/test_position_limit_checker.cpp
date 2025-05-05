#include <gtest/gtest.h>
#include "risk/pretrade/position_limit_checker.hpp"
#include "risk/portfolio/position_tracker.hpp"
#include <memory>

class PositionLimitCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create position tracker with some existing positions
        positionTracker = std::make_shared<PositionTracker>();
        positionTracker->updatePosition("binance", "BTC-USDT", "strategy1", 1.5, 45000.0);
        positionTracker->updatePosition("ftx", "ETH-USDT", "strategy2", 10.0, 3000.0);
        
        // Create limit manager with some limits
        limitManager = std::make_shared<LimitManager>();
        limitManager->setAssetLimit("BTC-USDT", 2.0);
        limitManager->setAssetLimit("ETH-USDT", 20.0);
        limitManager->setStrategyLimit("strategy1", 100000.0);
        limitManager->setExchangeLimit("binance", 200000.0);
        
        // Create position limit checker
        checker = std::make_unique<PositionLimitChecker>(positionTracker, limitManager);
    }

    std::shared_ptr<PositionTracker> positionTracker;
    std::shared_ptr<LimitManager> limitManager;
    std::unique_ptr<PositionLimitChecker> checker;
};

TEST_F(PositionLimitCheckerTest, CheckAssetLimit) {
    // Test within limit
    OrderRequest order1;
    order1.tradingPair = "BTC-USDT";
    order1.exchange = "binance";
    order1.strategy = "strategy1";
    order1.side = OrderSide::BUY;
    order1.quantity = 0.4; // Current: 1.5, Limit: 2.0, New: 1.9
    
    auto result1 = checker->checkPositionLimit(order1);
    EXPECT_TRUE(result1.approved);
    
    // Test exceeding limit
    OrderRequest order2;
    order2.tradingPair = "BTC-USDT";
    order2.exchange = "binance";
    order2.strategy = "strategy1";
    order2.side = OrderSide::BUY;
    order2.quantity = 0.6; // Current: 1.5, Limit: 2.0, New: 2.1
    
    auto result2 = checker->checkPositionLimit(order2);
    EXPECT_FALSE(result2.approved);
    EXPECT_EQ(result2.rejectionReason, "Asset position limit exceeded");
}

TEST_F(PositionLimitCheckerTest, CheckStrategyLimit) {
    // Set current prices for exposure calculation
    std::map<std::string, double> currentPrices = {
        {"BTC-USDT", 50000.0},
        {"ETH-USDT", 3000.0}
    };
    checker->setCurrentPrices(currentPrices);
    
    // Test within strategy limit
    OrderRequest order;
    order.tradingPair = "BTC-USDT";
    order.exchange = "binance";
    order.strategy = "strategy1";
    order.side = OrderSide::BUY;
    order.quantity = 1.0;
    order.price = 50000.0;
    
    auto result = checker->checkPositionLimit(order);
    EXPECT_TRUE(result.approved);
    
    // Test exceeding strategy limit with a very large order
    OrderRequest largeOrder;
    largeOrder.tradingPair = "BTC-USDT";
    largeOrder.exchange = "binance";
    largeOrder.strategy = "strategy1";
    largeOrder.side = OrderSide::BUY;
    largeOrder.quantity = 1.5;
    largeOrder.price = 50000.0; // Would add 75000 to exposure, exceeding limit
    
    auto largeResult = checker->checkPositionLimit(largeOrder);
    EXPECT_FALSE(largeResult.approved);
    EXPECT_EQ(largeResult.rejectionReason, "Strategy exposure limit exceeded");
}

TEST_F(PositionLimitCheckerTest, SellOrders) {
    // Selling should reduce position, so should not hit limits
    OrderRequest sellOrder;
    sellOrder.tradingPair = "BTC-USDT";
    sellOrder.exchange = "binance";
    sellOrder.strategy = "strategy1";
    sellOrder.side = OrderSide::SELL;
    sellOrder.quantity = 0.5;
    
    auto result = checker->checkPositionLimit(sellOrder);
    EXPECT_TRUE(result.approved);
    
    // Even a large sell should be allowed
    OrderRequest largeSellOrder;
    largeSellOrder.tradingPair = "BTC-USDT";
    largeSellOrder.exchange = "binance";
    largeSellOrder.strategy = "strategy1";
    largeSellOrder.side = OrderSide::SELL;
    largeSellOrder.quantity = 10.0; // Much larger than position
    
    auto largeResult = checker->checkPositionLimit(largeSellOrder);
    EXPECT_TRUE(largeResult.approved);
}

TEST_F(PositionLimitCheckerTest, NewAsset) {
    // Test with a new asset (no existing position)
    OrderRequest newAssetOrder;
    newAssetOrder.tradingPair = "SOL-USDT";
    newAssetOrder.exchange = "binance";
    newAssetOrder.strategy = "strategy1";
    newAssetOrder.side = OrderSide::BUY;
    newAssetOrder.quantity = 100.0;
    
    // Add a limit for this asset
    limitManager->setAssetLimit("SOL-USDT", 200.0);
    
    auto result = checker->checkPositionLimit(newAssetOrder);
    EXPECT_TRUE(result.approved);
    
    // Test exceeding limit for new asset
    OrderRequest largeNewAssetOrder;
    largeNewAssetOrder.tradingPair = "SOL-USDT";
    largeNewAssetOrder.exchange = "binance";
    largeNewAssetOrder.strategy = "strategy1";
    largeNewAssetOrder.side = OrderSide::BUY;
    largeNewAssetOrder.quantity = 250.0; // Exceeds 200 limit
    
    auto largeResult = checker->checkPositionLimit(largeNewAssetOrder);
    EXPECT_FALSE(largeResult.approved);
    EXPECT_EQ(largeResult.rejectionReason, "Asset position limit exceeded");
}