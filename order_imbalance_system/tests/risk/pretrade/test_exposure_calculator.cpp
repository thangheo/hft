/**
 * Unit tests for exposure calculator
 *
 * File: test_exposure_calculator.cpp
 * Created: 2025-05-05
 */

 #include <gtest/gtest.h>
 #include <gmock/gmock.h>
 
 #include "risk/pretrade/exposure_calculator.hpp"
 #include "risk/portfolio/position_tracker.hpp"
 #include "risk/interfaces/data_interface.hpp"
 #include "risk/risk_types.hpp"
 
 using namespace risk;
 using ::testing::Return;
 using ::testing::_;
 
 // Mock position tracker for testing
 class MockPositionTracker : public PositionTracker {
 public:
     MockPositionTracker() : PositionTracker(RiskConfig()) {}
     
     MOCK_METHOD(std::vector<Position>, getAllPositions, (), (const));
     MOCK_METHOD(std::vector<Position>, getPositionsForAsset, (const std::string&), (const));
     MOCK_METHOD(std::vector<Position>, getPositionsForStrategy, (const std::string&), (const));
     MOCK_METHOD(std::vector<Position>, getPositionsForExchange, (const std::string&), (const));
 };
 
 // Mock data interface for testing
 class MockDataInterface : public DataInterface {
 public:
     MockDataInterface() : DataInterface(RiskConfig()) {}
     
     MOCK_METHOD(MarketData, getMarketData, (), (const));
     MOCK_METHOD(double, getPrice, (const std::string&), (const));
     MOCK_METHOD(double, getVolatility, (const std::string&), (const));
 };
 
 class ExposureCalculatorTest : public ::testing::Test {
 protected:
     void SetUp() override {
         // Create a risk config with test limits
         RiskConfig config;
         ExposureLimitConfig limitConfig;
         
         // Set total exposure limit
         limitConfig.totalExposureLimit = 1000.0;
         
         // Set default limits
         limitConfig.defaultAssetExposureLimit = 100.0;
         limitConfig.defaultStrategyExposureLimit = 200.0;
         limitConfig.defaultExchangeExposureLimit = 500.0;
         
         // Set risk factor for volatility adjustment
         limitConfig.riskFactor = 2.0;
         
         // Set specific asset limits
         limitConfig.assetExposureLimits["BTC"] = 200.0;
         limitConfig.assetExposureLimits["ETH"] = 150.0;
         
         // Set specific strategy limits
         limitConfig.strategyExposureLimits["strategy1"] = 300.0;
         limitConfig.strategyExposureLimits["strategy2"] = 250.0;
         
         // Set specific exchange limits
         limitConfig.exchangeExposureLimits["exchange1"] = 600.0;
         limitConfig.exchangeExposureLimits["exchange2"] = 400.0;
         
         config.setExposureLimits(limitConfig);
         
         // Create mock objects
         mockPositionTracker = std::make_shared<MockPositionTracker>();
         mockDataInterface = std::make_shared<MockDataInterface>();
         
         // Set up mock market data
         MarketData marketData;
         marketData.prices["BTC/USDT"] = 50000.0;
         marketData.prices["ETH/USDT"] = 2000.0;
         marketData.volatilities["BTC/USDT"] = 0.2;  // 20% volatility
         marketData.volatilities["ETH/USDT"] = 0.15;  // 15% volatility
         
         EXPECT_CALL(*mockDataInterface, getMarketData())
             .WillOnce(Return(marketData));
         
         // Create exposure calculator with mocks
         exposureCalculator = std::make_unique<ExposureCalculator>(config, mockPositionTracker, mockDataInterface);
         exposureCalculator->initialize();
     }
     
     std::shared_ptr<MockPositionTracker> mockPositionTracker;
     std::shared_ptr<MockDataInterface> mockDataInterface;
     std::unique_ptr<ExposureCalculator> exposureCalculator;
 };
 
 // Test exposure calculation for a single position
 TEST_F(ExposureCalculatorTest, CalculatePositionExposure) {
     // Set up mock to return price and volatility
     EXPECT_CALL(*mockDataInterface, getPrice("BTC/USDT"))
         .WillOnce(Return(50000.0));
     
     EXPECT_CALL(*mockDataInterface, getVolatility("BTC/USDT"))
         .WillOnce(Return(0.2));
     
     // Set up mock to return positions
     Position position;
     position.tradingPair = TradingPair("BTC/USDT");
     position.asset = "BTC";
     position.quantity = 1.0;
     position.averageEntryPrice = 48000.0;  // Not used in calculation
     
     std::vector<Position> positions = {position};
     
     EXPECT_CALL(*mockPositionTracker, getAllPositions())
         .WillOnce(Return(positions));
     
     // Calculate total exposure
     double exposure = exposureCalculator->calculateTotalExposure();
     
     // Expected: position value * (1 + volatility * risk factor)
     // 50000 * 1.0 * (1 + 0.2 * 2.0) = 50000 * 1.4 = 70000
     EXPECT_NEAR(exposure, 70000.0, 0.01);
 }
 
 // Test checking exposure limits with trading signal that is within limits
 TEST_F(ExposureCalculatorTest, CheckExposureLimitWithinLimits) {
     // Create a trading signal
     TradingSignal signal;
     signal.id = "test-signal-1";
     signal.tradingPair = TradingPair("BTC/USDT");
     signal.direction = Direction::BUY;
     signal.targetSize = 0.5;  // 0.5 BTC
     signal.strategyId = "strategy1";
     signal.exchangeId = "exchange1";
     
     // Set up mock to return prices and volatility
     EXPECT_CALL(*mockDataInterface, getPrice("BTC/USDT"))
         .Times(4)  // Called multiple times during various calculations
         .WillRepeatedly(Return(50000.0));
     
     EXPECT_CALL(*mockDataInterface, getVolatility("BTC/USDT"))
         .WillRepeatedly(Return(0.2));
     
     // Set up mock to return current exposures
     // Empty positions means zero current exposure
     EXPECT_CALL(*mockPositionTracker, getAllPositions())
         .WillOnce(Return(std::vector<Position>()));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForAsset("BTC"))
         .WillOnce(Return(std::vector<Position>()));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForStrategy("strategy1"))
         .WillOnce(Return(std::vector<Position>()));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForExchange("exchange1"))
         .WillOnce(Return(std::vector<Position>()));
     
     // Check the exposure limit
     std::vector<RiskViolation> violations;
     bool result = exposureCalculator->checkExposureLimit(signal, violations);
     
     // Should pass all checks
     // New exposure: 0.5 BTC * 50000 * (1 + 0.2 * 2.0) = 35000
     // All limits are higher than this
     EXPECT_TRUE(result);
     EXPECT_TRUE(violations.empty());
 }
 
 // Test exceeding asset exposure limit
 TEST_F(ExposureCalculatorTest, ExceedAssetExposureLimit) {
     // Create a trading signal
     TradingSignal signal;
     signal.id = "test-signal-2";
     signal.tradingPair = TradingPair("BTC/USDT");
     signal.direction = Direction::BUY;
     signal.targetSize = 3.0;  // 3 BTC
     signal.strategyId = "strategy1";
     signal.exchangeId = "exchange1";
     
     // Set up mock to return prices and volatility
     EXPECT_CALL(*mockDataInterface, getPrice("BTC/USDT"))
         .Times(4)
         .WillRepeatedly(Return(50000.0));
     
     EXPECT_CALL(*mockDataInterface, getVolatility("BTC/USDT"))
         .WillRepeatedly(Return(0.2));
     
     // Set up mock to return current exposures
     // Current BTC exposure is already 100000
     Position position;
     position.tradingPair = TradingPair("BTC/USDT");
     position.asset = "BTC";
     position.quantity = 2.0;  // 2 BTC
     position.averageEntryPrice = 49000.0;
     position.strategyId = "strategy1";
     position.exchangeId = "exchange1";
     
     std::vector<Position> positions = {position};
     
     EXPECT_CALL(*mockPositionTracker, getAllPositions())
         .WillOnce(Return(positions));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForAsset("BTC"))
         .WillOnce(Return(positions));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForStrategy("strategy1"))
         .WillOnce(Return(positions));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForExchange("exchange1"))
         .WillOnce(Return(positions));
     
     // Check the exposure limit
     std::vector<RiskViolation> violations;
     bool result = exposureCalculator->checkExposureLimit(signal, violations);
     
     // Should fail the asset exposure check
     // Current asset exposure: 2 BTC * 50000 * (1 + 0.2 * 2.0) = 140000
     // New trade exposure: 3 BTC * 50000 * (1 + 0.2 * 2.0) = 210000
     // Total would be 350000, exceeding BTC limit of 200000
     EXPECT_FALSE(result);
     EXPECT_EQ(violations.size(), 1);
     EXPECT_EQ(violations[0].violationType, "ASSET_EXPOSURE_LIMIT_EXCEEDED");
     EXPECT_EQ(violations[0].limitValue, 200000.0);
     EXPECT_NEAR(violations[0].actualValue, 350000.0, 0.01);
 }
 
 // Test with OrderRequest instead of TradingSignal
 TEST_F(ExposureCalculatorTest, CheckExposureLimitWithOrderRequest) {
     // Create an order request
     OrderRequest orderRequest;
     orderRequest.clientOrderId = "test-order-1";
     orderRequest.tradingPair = TradingPair("BTC/USDT");
     orderRequest.side = OrderSide::BUY;
     orderRequest.quantity = 0.5;  // 0.5 BTC
     orderRequest.type = OrderType::LIMIT;
     orderRequest.price = 49000.0;
     orderRequest.strategyId = "strategy1";
     orderRequest.exchangeId = "exchange1";
     
     // Set up mock to return prices and volatility
     EXPECT_CALL(*mockDataInterface, getPrice("BTC/USDT"))
         .Times(4)
         .WillRepeatedly(Return(50000.0));
     
     EXPECT_CALL(*mockDataInterface, getVolatility("BTC/USDT"))
         .WillRepeatedly(Return(0.2));
     
     // Set up mock to return current exposures
     // Empty positions means zero current exposure
     EXPECT_CALL(*mockPositionTracker, getAllPositions())
         .WillOnce(Return(std::vector<Position>()));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForAsset("BTC"))
         .WillOnce(Return(std::vector<Position>()));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForStrategy("strategy1"))
         .WillOnce(Return(std::vector<Position>()));
     
     EXPECT_CALL(*mockPositionTracker, getPositionsForExchange("exchange1"))
         .WillOnce(Return(std::vector<Position>()));
     
     // Check the exposure limit
     std::vector<RiskViolation> violations;
     bool result = exposureCalculator->checkExposureLimit(orderRequest, violations);
     
     // Should pass all checks
     EXPECT_TRUE(result);
     EXPECT_TRUE(violations.empty());
 }
 
 // Test updating exposure limits
 TEST_F(ExposureCalculatorTest, UpdateExposureLimits) {
     // Get current limits
     ExposureLimitConfig originalLimits = exposureCalculator->getExposureLimits();
     
     // Modify limits
     ExposureLimitConfig newLimits = originalLimits;
     newLimits.totalExposureLimit = 2000.0;
     newLimits.assetExposureLimits["BTC"] = 300.0;
     
     // Update limits
     bool result = exposureCalculator->updateExposureLimits(newLimits);
     EXPECT_TRUE(result);
     
     // Check that limits were updated
     ExposureLimitConfig updatedLimits = exposureCalculator->getExposureLimits();
     EXPECT_EQ(updatedLimits.totalExposureLimit, 2000.0);
     EXPECT_EQ(updatedLimits.assetExposureLimits.at("BTC"), 300.0);
 }
 
 int main(int argc, char** argv) {
     ::testing::InitGoogleTest(&argc, argv);
     return RUN_ALL_TESTS();
 }