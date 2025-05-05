#include <gtest/gtest.h>
#include "risk/service/risk_validation_service.hpp"
#include "risk/pretrade/pre_trade_risk_filter.hpp"
#include <memory>

// Mock PreTradeRiskFilter for testing
class MockPreTradeRiskFilter : public PreTradeRiskFilter {
public:
    ValidationResult validateSignal(const TradingSignal& signal) override {
        ValidationResult result;
        result.signalId = signal.id;
        
        // Approve signals with strength <= 0.8, reject others
        if (signal.strength <= 0.8) {
            result.approved = true;
        } else {
            result.approved = false;
            result.violations.push_back({"MOCK_VIOLATION", "Signal strength too high", 0.8, signal.strength});
        }
        
        return result;
    }
    
    ValidationResult validateOrder(const OrderRequest& order) override {
        ValidationResult result;
        result.orderId = order.orderId;
        
        // Approve orders with quantity <= 1.0, reject others
        if (order.quantity <= 1.0) {
            result.approved = true;
        } else {
            result.approved = false;
            result.violations.push_back({"MOCK_VIOLATION", "Order size too large", 1.0, order.quantity});
        }
        
        return result;
    }
};

class RiskValidationServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock risk filter
        mockFilter = std::make_shared<MockPreTradeRiskFilter>();
        
        // Create risk validation service with mock filter
        service = std::make_unique<RiskValidationService>(mockFilter);
    }

    std::shared_ptr<MockPreTradeRiskFilter> mockFilter;
    std::unique_ptr<RiskValidationService> service;
};

TEST_F(RiskValidationServiceTest, ValidateSignal) {
    // Create valid signal
    TradingSignal validSignal;
    validSignal.id = "signal1";
    validSignal.strength = 0.7;
    
    // Validate signal
    ValidationResult validResult = service->validateSignal(validSignal);
    EXPECT_TRUE(validResult.approved);
    EXPECT_EQ(validResult.signalId, "signal1");
    
    // Create invalid signal
    TradingSignal invalidSignal;
    invalidSignal.id = "signal2";
    invalidSignal.strength = 0.9;
    
    // Validate signal
    ValidationResult invalidResult = service->validateSignal(invalidSignal);
    EXPECT_FALSE(invalidResult.approved);
    EXPECT_EQ(invalidResult.signalId, "signal2");
    EXPECT_EQ(invalidResult.violations.size(), 1);
    EXPECT_EQ(invalidResult.violations[0].violationType, "MOCK_VIOLATION");
}

TEST_F(RiskValidationServiceTest, ValidateOrder) {
    // Create valid order
    OrderRequest validOrder;
    validOrder.orderId = "order1";
    validOrder.quantity = 0.5;
    
    // Validate order
    ValidationResult validResult = service->validateOrder(validOrder);
    EXPECT_TRUE(validResult.approved);
    EXPECT_EQ(validResult.orderId, "order1");
    
    // Create invalid order
    OrderRequest invalidOrder;
    invalidOrder.orderId = "order2";
    invalidOrder.quantity = 1.5;
    
    // Validate order
    ValidationResult invalidResult = service->validateOrder(invalidOrder);
    EXPECT_FALSE(invalidResult.approved);
    EXPECT_EQ(invalidResult.orderId, "order2");
    EXPECT_EQ(invalidResult.violations.size(), 1);
    EXPECT_EQ(invalidResult.violations[0].violationType, "MOCK_VIOLATION");
}

TEST_F(RiskValidationServiceTest, GetRiskParameters) {
    // Test getting risk parameters
    RiskParameterRequest request;
    request.exchangeId = "binance";
    request.tradingPair = "BTC-USDT";
    request.strategyId = "strategy1";
    
    RiskParameters params = service->getRiskParameters(request);
    
    // Verify default parameters
    EXPECT_GE(params.maxPositionSize, 0.0);
    EXPECT_GE(params.maxOrderSize, 0.0);
    EXPECT_GE(params.maxExposure, 0.0);
}

TEST_F(RiskValidationServiceTest, UpdateRiskParameters) {
    // Test updating risk parameters
    RiskParameterUpdate update;
    update.exchangeId = "binance";
    update.tradingPair = "BTC-USDT";
    update.maxPositionSize = 2.0;
    update.maxOrderSize = 1.0;
    update.maxExposure = 100000.0;
    
    UpdateResult result = service->updateRiskParameters(update);
    EXPECT_TRUE(result.success);
    
    // Verify updated parameters
    RiskParameterRequest request;
    request.exchangeId = "binance";
    request.tradingPair = "BTC-USDT";
    
    RiskParameters params = service->getRiskParameters(request);
    EXPECT_DOUBLE_EQ(params.maxPositionSize, 2.0);
    EXPECT_DOUBLE_EQ(params.maxOrderSize, 1.0);
    EXPECT_DOUBLE_EQ(params.maxExposure, 100000.0);
}