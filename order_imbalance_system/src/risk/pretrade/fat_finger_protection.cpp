/**
 * @file fat_finger_protection.cpp
 * @brief Modern C++20 implementation of fat finger protection
 * 
 * High-performance implementation using modern C++ features, advanced
 * statistical analysis, and optimal concurrency patterns.
 * 
 * @author Trading System Team  
 * @date May 28, 2025
 */

#include "risk/pretrade/fat_finger_protection.hpp"
#include <algorithm>
#include <execution>
#include <format>

namespace trading::risk {

// OrderPatternAnalyzer Implementation

void OrderPatternAnalyzer::addSample(double value) noexcept {
    if (!std::isfinite(value) || value <= 0.0) {
        return; // Skip invalid values
    }

    std::unique_lock lock{mutex_};
    
    samples_.push_back(value);
    if (samples_.size() > maxHistorySize_) {
        samples_.pop_front();
    }
    
    statsDirty_ = true;
    cachedStats_.reset();
}

OrderPatternAnalyzer::Statistics OrderPatternAnalyzer::getStatistics() const noexcept {
    std::shared_lock lock{mutex_};
    
    if (!statsDirty_ && cachedStats_) {
        return *cachedStats_;
    }
    
    // Upgrade to unique lock for cache update
    lock.unlock();
    std::unique_lock uniqueLock{mutex_};
    
    // Double-check pattern
    if (!statsDirty_ && cachedStats_) {
        return *cachedStats_;
    }
    
    cachedStats_ = calculateStatistics();
    statsDirty_ = false;
    
    return *cachedStats_;
}

double OrderPatternAnalyzer::getAnomalyScore(double value) const noexcept {
    const auto stats = getStatistics();
    
    if (!stats.isValid() || stats.stdDev == 0.0) {
        return 0.0; // Cannot determine anomaly without valid statistics
    }
    
    // Z-score based anomaly detection
    const double zScore = std::abs((value - stats.mean) / stats.stdDev);
    
    // Modified z-score using median for robustness
    const double modifiedZScore = 0.6745 * std::abs((value - stats.median) / stats.stdDev);
    
    // Return the more conservative (higher) score
    return std::max(zScore, modifiedZScore);
}

bool OrderPatternAnalyzer::isOutlier(double value, double thresholdSigmas) const noexcept {
    return getAnomalyScore(value) > thresholdSigmas;
}

OrderPatternAnalyzer::Statistics OrderPatternAnalyzer::calculateStatistics() const noexcept {
    if (samples_.empty()) {
        return {};
    }
    
    Statistics stats;
    stats.sampleSize = samples_.size();
    
    // Convert to vector for algorithms
    std::vector<double> sortedSamples(samples_.begin(), samples_.end());
    std::ranges::sort(sortedSamples);
    
    // Calculate mean
    stats.mean = std::reduce(std::execution::unseq, 
                           sortedSamples.begin(), sortedSamples.end()) / sortedSamples.size();
    
    // Calculate median
    const size_t n = sortedSamples.size();
    if (n % 2 == 0) {
        stats.median = (sortedSamples[n/2 - 1] + sortedSamples[n/2]) / 2.0;
    } else {
        stats.median = sortedSamples[n/2];
    }
    
    // Calculate quartiles
    stats.q25 = sortedSamples[n/4];
    stats.q75 = sortedSamples[3*n/4];
    
    // Calculate standard deviation
    const double variance = std::transform_reduce(
        std::execution::unseq,
        sortedSamples.begin(), sortedSamples.end(),
        0.0,
        std::plus<>(),
        [mean = stats.mean](double x) {
            const double diff = x - mean;
            return diff * diff;
        }
    ) / (sortedSamples.size() - 1);
    
    stats.stdDev = std::sqrt(variance);
    
    return stats;
}

// FatFingerProtection Implementation

FatFingerProtection::FatFingerProtection(
    std::shared_ptr<DataInterface> dataInterface,
    Config config
) noexcept 
    : dataInterface_(std::move(dataInterface))
    , config_(std::move(config)) 
{
    if (!config_.isValid()) {
        LOG_WARN("Invalid fat finger protection config provided, using defaults");
        config_ = Config{};
    }
}

std::expected<void, std::string> FatFingerProtection::initialize() noexcept {
    if (!dataInterface_) {
        return std::unexpected("DataInterface is null");
    }
    
    try {
        if (!dataInterface_->initialize()) {
            return std::unexpected("Failed to initialize DataInterface");
        }
        
        LOG_INFO("Fat finger protection initialized successfully with config: "
                "priceDeviation={:.2f}%, sizeThreshold={:.1f}σ, notionalLimit=${:.0f}",
                config_.priceDeviationThreshold * 100,
                config_.sizeAnomalyThreshold,
                config_.notionalValueThreshold);
        
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Initialization failed: {}", e.what()));
    }
}

bool FatFingerProtection::updateConfig(const Config& newConfig) noexcept {
    if (!newConfig.isValid()) {
        LOG_ERROR("Attempted to set invalid fat finger protection config");
        return false;
    }
    
    config_ = newConfig;
    
    LOG_INFO("Updated fat finger protection config: "
            "priceDeviation={:.2f}%, sizeThreshold={:.1f}σ, notionalLimit=${:.0f}",
            config_.priceDeviationThreshold * 100,
            config_.sizeAnomalyThreshold,
            config_.notionalValueThreshold);
    
    return true;
}

std::optional<OrderPatternAnalyzer::Statistics> 
FatFingerProtection::getOrderStatistics(std::string_view symbol) const noexcept {
    const std::string normalizedSymbol = normalizeSymbol(symbol);
    
    std::shared_lock lock{historyMutex_};
    
    if (auto it = orderAnalyzers_.find(normalizedSymbol); it != orderAnalyzers_.end()) {
        return it->second->getStatistics();
    }
    
    return std::nullopt;
}

bool FatFingerProtection::requiresAdditionalVerification(
    std::string_view symbol,
    double price,
    double quantity
) const noexcept {
    const double notionalValue = price * std::abs(quantity);
    return notionalValue > config_.notionalValueThreshold;
}

std::string FatFingerProtection::normalizeSymbol(std::string_view symbol) const noexcept {
    std::string result{symbol};
    std::ranges::transform(result, result.begin(), ::toupper);
    return result;
}

ValidationResult FatFingerProtection::validatePriceDeviation(
    std::string_view symbol,
    double orderPrice
) noexcept {
    auto marketPriceResult = getCurrentMarketPrice(symbol);
    
    if (!marketPriceResult) {
        ++rejectedOrders_;
        return std::unexpected(ViolationDetails{
            .code = ViolationCode::MARKET_DATA_UNAVAILABLE,
            .message = std::format("Cannot verify price for {}: {}", 
                                 symbol, marketPriceResult.error()),
            .actualValue = orderPrice,
            .severity = 0.8
        });
    }
    
    const double marketPrice = *marketPriceResult;
    const double deviation = std::abs((orderPrice - marketPrice) / marketPrice);
    
    if (deviation > config_.priceDeviationThreshold) {
        ++rejectedOrders_;
        return std::unexpected(ViolationDetails{
            .code = ViolationCode::PRICE_DEVIATION_EXCESSIVE,
            .message = std::format("Price deviation {:.2f}% exceeds threshold {:.2f}% for {}",
                                 deviation * 100, 
                                 config_.priceDeviationThreshold * 100,
                                 symbol),
            .expectedValue = config_.priceDeviationThreshold * 100,
            .actualValue = deviation * 100,
            .severity = std::min(1.0, deviation / config_.priceDeviationThreshold)
        });
    }
    
    return {};
}

ValidationResult FatFingerProtection::validateSizeAnomaly(
    std::string_view symbol,
    double orderSize
) noexcept {
    if (orderSize <= 0.0) {
        ++rejectedOrders_;
        return std::unexpected(ViolationDetails{
            .code = ViolationCode::SIZE_ANOMALY_DETECTED,
            .message = std::format("Invalid order size {:.6f} for {}", orderSize, symbol),
            .actualValue = orderSize,
            .severity = 1.0
        });
    }
    
    const std::string normalizedSymbol = normalizeSymbol(symbol);
    
    std::shared_lock lock{historyMutex_};
    
    auto it = orderAnalyzers_.find(normalizedSymbol);
    if (it == orderAnalyzers_.end()) {
        // No history available - allow but log
        LOG_DEBUG("No order history for {} - allowing order", symbol);
        return {};
    }
    
    const auto& analyzer = *it->second;
    const auto stats = analyzer.getStatistics();
    
    if (!stats.isValid() || stats.sampleSize < config_.minimumHistorySize) {
        LOG_DEBUG("Insufficient history for {} (samples: {}) - allowing order", 
                 symbol, stats.sampleSize);
        return {};
    }
    
    const double anomalyScore = analyzer.getAnomalyScore(orderSize);
    
    if (anomalyScore > config_.sizeAnomalyThreshold) {
        ++rejectedOrders_;
        return std::unexpected(ViolationDetails{
            .code = ViolationCode::SIZE_ANOMALY_DETECTED,
            .message = std::format("Order size {:.6f} is {:.1f}σ from typical size for {} "
                                 "(mean: {:.6f}, median: {:.6f})",
                                 orderSize, anomalyScore, symbol,
                                 stats.mean, stats.median),
            .expectedValue = config_.sizeAnomalyThreshold,
            .actualValue = anomalyScore,
            .severity = std::min(1.0, anomalyScore / config_.sizeAnomalyThreshold)
        });
    }
    
    return {};
}

ValidationResult FatFingerProtection::validateNotionalValue(
    std::string_view symbol,
    double price,
    double quantity
) noexcept {
    const double notionalValue = price * std::abs(quantity);
    
    if (notionalValue > config_.notionalValueThreshold) {
        if (config_.rejectHighNotionalOrders) {
            ++rejectedOrders_;
            return std::unexpected(ViolationDetails{
                .code = ViolationCode::NOTIONAL_VALUE_TOO_HIGH,
                .message = std::format("Notional value ${:.2f} exceeds limit ${:.2f} for {}",
                                     notionalValue, config_.notionalValueThreshold, symbol),
                .expectedValue = config_.notionalValueThreshold,
                .actualValue = notionalValue,
                .severity = std::min(1.0, notionalValue / config_.notionalValueThreshold)
            });
        } else {
            // Log but allow - requires additional verification
            LOG_WARN("High notional value order for {}: ${:.2f} (limit: ${:.2f}) - "
                    "requires additional verification",
                    symbol, notionalValue, config_.notionalValueThreshold);
        }
    }
    
    return {};
}

std::expected<double, std::string> FatFingerProtection::getCurrentMarketPrice(
    std::string_view symbol
) noexcept {
    try {
        const std::string symbolStr{symbol};
        const double price = dataInterface_->getCurrentPrice(symbolStr);
        
        if (!std::isfinite(price) || price <= 0.0) {
            return std::unexpected(std::format("Invalid market price: {}", price));
        }
        
        ++cacheHits_;
        return price;
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Market data error: {}", e.what()));
    }
}

OrderPatternAnalyzer& FatFingerProtection::getOrCreateAnalyzer(
    std::string_view symbol
) noexcept {
    const std::string normalizedSymbol = normalizeSymbol(symbol);
    
    std::unique_lock lock{historyMutex_};
    
    auto [it, inserted] = orderAnalyzers_.try_emplace(
        normalizedSymbol,
        std::make_unique<OrderPatternAnalyzer>()
    );
    
    return *it->second;
}

} // namespace trading::risk