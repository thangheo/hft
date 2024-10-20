// market_data_handler.cpp

#include "market_data_handler.h"
#include "adapters/binance_adapter.h"
// Include other exchange adapters as needed
#include <iostream>
// MarketDataHandler::MarketDataHandler() {}
MarketDataHandler::MarketDataHandler(std::shared_ptr<prometheus::Registry> registry) {
    // Initialize counters
    auto& market_data_messages_family = prometheus::BuildCounter()
        .Name("market_data_messages_total")
        .Help("Total number of market data messages received")
        .Register(*registry);
    market_data_messages_counter_ = &market_data_messages_family.Add({{"component", "MarketDataHandler"}});

    auto& market_data_errors_family = prometheus::BuildCounter()
        .Name("market_data_errors_total")
        .Help("Total number of market data errors")
        .Register(*registry);
    market_data_errors_counter_ = &market_data_errors_family.Add({{"component", "MarketDataHandler"}});
}

MarketDataHandler::~MarketDataHandler() {
    // Disconnect from all exchanges
    for (auto& pair : exchange_adapters_) {
        pair.second->disconnect();
    }
}

bool MarketDataHandler::connect(const std::string& exchange_name) {
    if (exchange_adapters_.find(exchange_name) != exchange_adapters_.end()) {
        // Already connected
        return true;
    }

    std::shared_ptr<IExchangeAdapter> adapter;

    if (exchange_name == "binance") {
        adapter = std::make_shared<BinanceAdapter>();
    } else if (exchange_name == "coinbase") {
        // adapter = std::make_shared<CoinbaseAdapter>();
    } else {
        std::cerr << "Unsupported exchange: " << exchange_name << std::endl;
        return false;
    }

    adapter->setDataCallback([this, exchange_name](const std::string& data) {
        onExchangeData(exchange_name, data);
    });

    if (adapter->connect()) {
        exchange_adapters_[exchange_name] = adapter;
        return true;
    } else {
        std::cerr << "Failed to connect to exchange: " << exchange_name << std::endl;
        return false;
    }
}

void MarketDataHandler::disconnect(const std::string& exchange_name) {
    auto it = exchange_adapters_.find(exchange_name);
    if (it != exchange_adapters_.end()) {
        it->second->disconnect();
        exchange_adapters_.erase(it);
    }
}

void MarketDataHandler::subscribe(const std::string& symbol) {
    for (auto& pair : exchange_adapters_) {
        pair.second->subscribe(symbol);
    }
}

void MarketDataHandler::unsubscribe(const std::string& symbol) {
    for (auto& pair : exchange_adapters_) {
        pair.second->unsubscribe(symbol);
    }
}

void MarketDataHandler::setMarketDataCallback(const std::function<void(const MarketData&)>& callback) {
    market_data_callback_ = callback;
}

void MarketDataHandler::onExchangeData(const std::string& exchange_name, const std::string& data) {
    // Parse the data into internal MarketData structure
    MarketData market_data;
    // ... parsing logic ...
    //define error
    bool error = false;
    // error = !market_data.parse(data);
    // Increment message counter
    market_data_messages_counter_->Increment();

    // Handle errors
    if (error) {
        market_data_errors_counter_->Increment();
    }
    // Set the exchange name
    market_data.exchange = exchange_name;
    
    // Store market data
    if (storage_manager_) {
        storage_manager_->storeRealTimeMarketData(market_data);
        storage_manager_->storeHistoricalMarketData(market_data);
    }
     // Cache the market data
    if (real_time_db_) {
        real_time_db_->cacheMarketData(market_data);
    }
    // Invoke the callback
    if (market_data_callback_) {
        market_data_callback_(market_data);
    }
}
