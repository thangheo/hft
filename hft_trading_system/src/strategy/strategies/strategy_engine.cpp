
void StrategyEngine::onTick() {
    MarketData market_data;
    if (real_time_db_->getCachedMarketData("btcusdt", market_data)) {
        // Use the cached market data
        processMarketData(market_data);
    } else {
        // Handle cache miss, possibly wait for the next data update
    }
}
