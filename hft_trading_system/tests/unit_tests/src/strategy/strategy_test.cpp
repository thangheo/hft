#include <gtest/gtest.h>
#include "strategies/mean_reversion_strategy.h"

TEST(MeanReversionStrategyTest, BasicOperation) {
    MeanReversionStrategy strategy;
    ASSERT_TRUE(strategy.initialize(""));

    MarketData data;
    data.symbol = "btcusdt";
    data.bids.push_back({50000.0, 1.0});
    data.asks.push_back({50010.0, 1.0});

    strategy.onMarketData(data);

    // Verify that the strategy's internal state is as expected
    // For example, check if the moving average is updated correctly
}
