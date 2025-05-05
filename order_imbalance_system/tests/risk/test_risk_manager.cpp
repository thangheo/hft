#include <gtest/gtest.h>
#include "risk/risk_manager.hpp"
#include "risk/pretrade/pre_trade_risk_filter.hpp"
#include "risk/posttrade/post_trade_monitor.hpp"
#include "risk/portfolio/portfolio_manager.hpp"
#include <memory>

// Mock classes for testing
class MockPreTradeRiskFilter : public PreTradeRiskFilter {
public:
    ValidationResult validateSignal(const TradingSignal& signal) override {
        ValidationResult result;
        result.signalId = signal.id;
        result.approved = (signal.strength <= 0.8);
        return result;
    }
    
    ValidationResult validateOrder(const OrderRequest& order) override {
        ValidationResult result;
        result.orderId = order.orderId;
        result.approved = (order.quantity <= 1.0);
        return result;
    }
};

class MockPostTradeMonitor : public PostTradeMonitor {
public:
    void processExecution(const ExecutionReport& report) override {
        lastProcessedExecution = report;
    }
    
    ExecutionReport lastProcessedExecution;
};

class MockPortfolioManager : public PortfolioManager {
public:
    Position getPosition(const std::string& exchange, const std::string& tradingPair, 
                        const std::string& strategy) override {
        Position pos;
        pos.exchangeId = exchange;
        pos.tradingPair = tradingPair;
        pos.strategyId = strategy;
        pos.quantity = 1.0; // Fixed test value
        return pos;
    }
    
    void updatePosition(const Position& position) override {
        lastUpdatedPosition = position;
    }
    
    Position lastUpdatedPosition;
};

class RiskManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock components
        mockPreTradeFilter = std::make_shared<MockPreTradeRiskFilter>();
        mockPostTradeMonitor = std::make_shared<MockPostTradeMonitor>();
        mockPortfolioManager = std::make_shared<MockPortfolioManager>();
        
        // Create risk manager with mock components
        riskManager = std::make_unique<RiskManager>(
            mockPreTradeFilter,
            mockPostTradeMonitor,
            mockPortfolioManager
        );
    }

    std::shared_ptr<MockPreTradeRiskFilter> mockPreTradeFilter;
    std::shared_ptr<MockPostTradeMonitor> mockPostTradeMonitor;
    std::shared_ptr<MockPortfolioManager> mockPortfolioManager;
    std::unique_ptr<RiskManager> riskManager;
};

TEST_F(RiskManagerTest, ValidateSignal) {
    // Create valid signal
    TradingSignal validSignal;
    validSignal.id = "signal1";
    validSignal.strength = 0.7;
    
    // Validate signal
    ValidationResult validResult = riskManager->validateSignal(validSignal);
    EXPECT_TRUE(validResult.approved);
    
    // Create invalid signal
    TradingSignal invalidSignal;
    invalidSignal.id = "signal2";
    invalidSignal.strength = 0.9;
    
    // Validate signal
    ValidationResult invalidResult = riskManager->validateSignal(invalidSignal);
    EXPECT_FALSE(invalidResult.approved);
}

TEST_F(RiskManagerTest, ValidateOrder) {
    // Create valid order
    OrderRequest validOrder;
    validOrder.orderId = "order1";
    validOrder.quantity = 0.5;
    
    // Validate order
    ValidationResult validResult = riskManager->validateOrder(validOrder);
    EXPECT_TRUE(validResult.approved);
    
    // Create invalid order
    OrderRequest invalidOrder;
    invalidOrder.orderId = "order2";
    invalidOrder.quantity = 1.5;
    
    // Validate order
    ValidationResult invalidResult = riskManager->validateOrder(invalidOrder);
    EXPECT_FALSE(invalidResult.approved);
}

TEST_F(RiskManagerTest, ProcessExecution) {
    // Create execution report
    ExecutionReport report;
    report.orderId = "order1";
    report.exchangeId = "binance";
    report.tradingPair = "BTC-USDT";
    report.side = OrderSide::BUY;
    report.quantity = 0.5;
    report.price = 50000.0;
    
    // Process execution
    riskManager->processExecution(report);
    
    // Verify that post-trade monitor received the report
    EXPECT_EQ(mockPostTradeMonitor->lastProcessedExecution.orderId, "order1");
    EXPECT_EQ(mockPostTradeMonitor->lastProcessedExecution.exchangeId, "binance");
    
    // Create a filled execution to test position updates
    ExecutionReport filledReport;
    filledReport.orderId = "order2";
    filledReport.exchangeId = "binance";
    filledReport.tradingPair = "BTC-USDT";
    filledReport.strategyId = "strategy1";
    filledReport.side = OrderSide::BUY;
    filledReport.quantity = 0.5;
    filledReport.price = 50000.0;
    filledReport.status = OrderStatus::FILLED;
    
    // Process filled execution
    riskManager->processExecution(filledReport);
    
    // Verify that portfolio manager received position update
    EXPECT_EQ(mockPortfolioManager->lastUpdatedPosition.exchangeId, "binance");
    EXPECT_EQ(mockPortfolioManager->lastUpdatedPosition.tradingPair, "BTC-USDT");
    EXPECT_EQ(mockPortfolioManager->lastUpdatedPosition.strategyId, "strategy1");
}

TEST_F(RiskManagerTest, GetPosition) {
    // Get position
    Position position = riskManager->getPosition("binance", "BTC-USDT", "strategy1");
    
    // Verify position
    EXPECT_EQ(position.exchangeId, "binance");
    EXPECT_EQ(position.tradingPair, "BTC-USDT");
    EXPECT_EQ(position.strategyId, "strategy1");
    EXPECT_DOUBLE_EQ(position.quantity, 1.0); // From mock
}

TEST_F(RiskManagerTest, EmergencyActions) {
    // Test emergency order cancellation
    bool cancelResult = riskManager->emergencyCancelOrders("strategy1");
    EXPECT_TRUE(cancelResult);
    
    // Test circuit breaker
    bool breakerResult = riskManager->activateCircuitBreaker(CircuitBreakerLevel::CRITICAL);
    EXPECT_TRUE(breakerResult);
    
    // Check circuit breaker status
    CircuitBreakerStatus status = riskManager->getCircuitBreakerStatus();
    EXPECT_EQ(status.level, CircuitBreakerLevel::CRITICAL);
    EXPECT_TRUE(status.active);
}