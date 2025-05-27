/**
 * @file benchmark_exposure_calculator.cpp
 * @brief Performance benchmarks for ExposureCalculator
 * 
 * Measures performance characteristics under various load conditions
 * to ensure HFT latency requirements are met.
 * 
 * @author Trading System Team
 * @date May 27, 2025
 */

#include <benchmark/benchmark.h>
#include "risk/pretrade/exposure_calculator.hpp"
#include "risk/pretrade/trading_signal.hpp"
#include <memory>
#include <random>
#include <vector>

using namespace trading::risk;

// Mock implementations for benchmarking
class BenchmarkPositionTracker : public PositionTracker {
public:
    BenchmarkPositionTracker(size_t numPositions) {
        // Generate test positions
        TradingPair btcPair{"BTC", "USDT", "BTC/USDT"};
        StrategyId strategy{"benchmark_strategy", "v1"};
        
        std::mt19937 gen(42); // Fixed seed for reproducibility
        std::uniform_real_distribution<> sizeDist(0.1, 2.0);
        std::uniform_real_distribution<> priceDist(45000.0, 55000.0);
        
        for (size_t i = 0; i < numPositions; ++i) {
            positions_.emplace_back(
                core::ExchangeId::binance(),
                btcPair,
                strategy,
                sizeDist(gen),
                priceDist(gen)
            );
        }
    }
    
    std::vector<Position> getAllPositions() const override {
        return positions_;
    }
    
    std::vector<Position> getPositionsForAsset(const std::string& assetId) const override {
        std::vector<Position> result;
        for (const auto& pos : positions_) {
            if (pos.getTradingPair().base == assetId) {
                result.push_back(pos);
            }
        }
        return result;
    }
    
    std::vector<Position> getPositionsForStrategy(const std::string& strategyId) const override {
        return positions_; // Simplified for benchmark
    }
    
    std::vector<Position> getPositionsForExchange(const core::ExchangeId& exchangeId) const override {
        return positions_; // Simplified for benchmark
    }

private:
    std::vector<Position> positions_;
};

class BenchmarkDataInterface : public DataInterface {
public:
    bool initialize() override { return true; }
    
    std::optional<core::MarketData> getMarketData(
        const std::string& tradingPair,
        const std::optional<std::string>& exchangeId = std::nullopt
    ) override {
        core::MarketData data;
        data.tradingPair = tradingPair;
        data.price = 50000.0;
        return data;
    }
    
    double getCurrentPrice(
        const std::string& tradingPair,
        const std::optional<std::string>& exchangeId = std::nullopt
    ) override {
        return 50000.0; // Fixed price for benchmark consistency
    }
    
    double getVolatility(
        const std::string& tradingPair,
        const std::string& timeframe,
        const std::optional<std::string>& exchangeId = std::nullopt
    ) override {
        return 0.15; // Fixed 15% volatility
    }
    
    // Implement other required methods as no-ops for benchmarking
    std::vector<std::pair<Timestamp, double>> getHistoricalPrices(
        const std::string&, const std::string&, int, const std::optional<std::string>&
    ) override { return {}; }
    
    std::vector<OrderRequest> getOrderHistory(
        const core::ExchangeId&, const std::string&, int
    ) override { return {}; }
    
    std::map<std::string, double> getExchangeBalances(
        const core::ExchangeId&
    ) override { return {}; }
    
    void registerMarketDataCallback(
        const std::function<void(const core::MarketData&)>&
    ) override {}
    
    bool subscribeMarketData(
        const std::string&, const std::optional<std::string>&
    ) override { return true; }
    
    bool unsubscribeMarketData(
        const std::string&, const std::optional<std::string>&
    ) override { return true; }
    
    std::map<std::string, double> getTradingFees(
        const core::ExchangeId&, const std::optional<std::string>&
    ) override { return {}; }
};

// Global benchmark setup
class ExposureCalculatorFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        size_t numPositions = state.range(0);
        
        positionTracker = std::make_shared<BenchmarkPositionTracker>(numPositions);
        dataInterface = std::make_shared<BenchmarkDataInterface>();
        
        ExposureCalculator::Config config;
        config.cacheExpiryTime = std::chrono::milliseconds(1000); // Longer cache for benchmarks
        config.enableSIMDOptimizations = true;
        config.enableAsyncCalculations = false; // Synchronous for consistent timing
        
        exposureCalculator = std::make_unique<ExposureCalculator>(
            positionTracker, dataInterface, config
        );
        
        RiskConfig riskConfig;
        auto initResult = exposureCalculator->initialize(riskConfig);
        if (!initResult.has_value()) {
            throw std::runtime_error("Failed to initialize exposure calculator");
        }
        
        // Pre-generate signals for consistent benchmarking
        std::mt19937 gen(123);
        std::uniform_real_distribution<> sizeDist(0.1, 2.0);
        std::vector<std::string> tradingPairs = {"BTC/USDT", "ETH/USDT", "ADA/USDT", "DOT/USDT"};
        std::vector<std::string> strategies = {"strategy_1", "strategy_2", "strategy_3"};
        
        for (int i = 0; i < 10000; ++i) {
            signals_.emplace_back(
                std::format("signal_{}", i),
                tradingPairs[i % tradingPairs.size()],
                core::ExchangeId::binance(),
                (i % 2 == 0) ? core::Direction::BUY : core::Direction::SELL,
                sizeDist(gen),
                strategies[i % strategies.size()]
            );
        }
    }
    
    void TearDown(const benchmark::State& state) override {
        exposureCalculator.reset();
        positionTracker.reset();
        dataInterface.reset();
    }

protected:
    std::shared_ptr<BenchmarkPositionTracker> positionTracker;
    std::shared_ptr<BenchmarkDataInterface> dataInterface;
    std::unique_ptr<ExposureCalculator> exposureCalculator;
    std::vector<TradingSignal> signals_;
};

// Benchmark single exposure validation
BENCHMARK_DEFINE_F(ExposureCalculatorFixture, SingleValidation)(benchmark::State& state) {
    size_t signalIndex = 0;
    
    for (auto _ : state) {
        const auto& signal = signals_[signalIndex % signals_.size()];
        auto result = exposureCalculator->validateExposure(signal);
        benchmark::DoNotOptimize(result);
        signalIndex++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark with different numbers of positions
BENCHMARK_REGISTER_F(ExposureCalculatorFixture, SingleValidation)
    ->Args({0})      // No positions
    ->Args({10})     // Small portfolio
    ->Args({100})    // Medium portfolio
    ->Args({1000})   // Large portfolio
    ->Args({10000})  // Very large portfolio
    ->Unit(benchmark::kNanosecond)
    ->Iterations(100000);

// Benchmark total exposure calculation
BENCHMARK_DEFINE_F(ExposureCalculatorFixture, TotalExposureCalculation)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = exposureCalculator->calculateTotalExposure();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(ExposureCalculatorFixture, TotalExposureCalculation)
    ->Args({100})
    ->Args({1000})
    ->Args({10000})
    ->Unit(benchmark::kNanosecond);

// Benchmark cache performance
BENCHMARK_DEFINE_F(ExposureCalculatorFixture, CachePerformance)(benchmark::State& state) {
    // Use the same signal repeatedly to test cache hits
    const auto& signal = signals_[0];
    
    for (auto _ : state) {
        auto result = exposureCalculator->validateExposure(signal);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    // Report cache statistics
    const auto& stats = exposureCalculator->getPerformanceStats();
    state.counters["CacheHits"] = stats.cacheHits.load();
    state.counters["CacheMisses"] = stats.cacheMisses.load();
    state.counters["CacheHitRate"] = 
        static_cast<double>(stats.cacheHits.load()) / 
        (stats.cacheHits.load() + stats.cacheMisses.load());
}

BENCHMARK_REGISTER_F(ExposureCalculatorFixture, CachePerformance)
    ->Args({100})
    ->Unit(benchmark::kNanosecond)
    ->Iterations(10000);

// Benchmark multithreaded performance
BENCHMARK_DEFINE_F(ExposureCalculatorFixture, MultithreadedValidation)(benchmark::State& state) {
    const int numThreads = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<int> completedValidations{0};
        
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, t]() {
                const auto& signal = signals_[t % signals_.size()];
                auto result = exposureCalculator->validateExposure(signal);
                benchmark::DoNotOptimize(result);
                completedValidations.fetch_add(1);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        benchmark::DoNotOptimize(completedValidations.load());
    }
    
    state.SetItemsProcessed(state.iterations() * numThreads);
}

BENCHMARK_REGISTER_F(ExposureCalculatorFixture, MultithreadedValidation)
    ->Args({1})
    ->Args({2})
    ->Args({4})
    ->Args({8})
    ->Args({16})
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime();

// Benchmark memory allocation patterns
BENCHMARK_DEFINE_F(ExposureCalculatorFixture, MemoryAllocation)(benchmark::State& state) {
    for (auto _ : state) {
        // Create new signals to test memory allocation patterns
        TradingSignal signal{
            "benchmark_signal",
            "BTC/USDT",
            core::ExchangeId::binance(),
            core::Direction::BUY,
            1.0,
            "benchmark_strategy"
        };
        
        auto result = exposureCalculator->validateExposure(signal);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(ExposureCalculatorFixture, MemoryAllocation)
    ->Args({100})
    ->Unit(benchmark::kNanosecond);

// Custom main to run benchmarks with appropriate settings
int main(int argc, char** argv) {
    // Initialize benchmark with custom settings
    benchmark::Initialize(&argc, argv);
    
    // Set CPU affinity for more consistent results (Linux)
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset); // Pin to CPU 0
    sched_setaffinity(0, sizeof(cpuset), &cpuset);
#endif
    
    // Run benchmarks
    benchmark::RunSpecifiedBenchmarks();
    
    // Print summary
    std::cout << "\n=== Benchmark Summary ===" << std::endl;
    std::cout << "Target latency for HFT: < 10 microseconds" << std::endl;
    std::cout << "All benchmarks should show sub-microsecond performance for single validations" << std::endl;
    std::cout << "Cache hit rates should exceed 90% for repeated validations" << std::endl;
    
    return 0;
}