// exchange_calibration.cpp
#include "strategy/optimization/exchange_calibration.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <cmath>
#include <nlohmann/json.hpp>

namespace trading {
namespace strategy {

ExchangeCalibration::ExchangeCalibration() {
    // Initialize with empty parameters
}
ExchangeCalibration::ExchangeCalibration(const ExchangeCalibrationConfig& config) {
    // Initialize with configuration
    initialize(config);
}
bool ExchangeCalibration::initialize(const ExchangeCalibrationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    baseParameters_ = config.baseParameters;

    for (const auto& exchange : config.exchanges) {
        exchangeProfiles_[exchange] = ExchangeMetrics();
        exchangeParameters_[exchange] = baseParameters_;
        calibrationTimes_[exchange] = std::chrono::system_clock::now();
    }

    return true;
}

void ExchangeCalibration::updateExchangeProfile(const ExchangeId& exchange, const ExchangeMetrics& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Store or update exchange metrics
    exchangeProfiles_[exchange] = metrics;
    
    // Update calibration time
    calibrationTimes_[exchange] = std::chrono::system_clock::now();
}

ParameterSet ExchangeCalibration::getExchangeParameters(const ExchangeId& exchange) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we have parameters for this exchange
    auto it = exchangeParameters_.find(exchange);
    if (it != exchangeParameters_.end()) {
        return it->second;
    }
    
    // If not calibrated, return base parameters
    return baseParameters_;
}

void ExchangeCalibration::calibrateParameters(const ExchangeId& exchange) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we have metrics for this exchange
    auto it = exchangeProfiles_.find(exchange);
    if (it == exchangeProfiles_.end()) {
        // Cannot calibrate without metrics
        return;
    }
    
    // Apply exchange-specific adjustments to base parameters
    ParameterSet calibratedParams = applyExchangeAdjustments(baseParameters_, it->second);
    
    // Store calibrated parameters
    exchangeParameters_[exchange] = calibratedParams;
    
    // Update calibration timestamp
    calibrationTimes_[exchange] = std::chrono::system_clock::now();
}

bool ExchangeCalibration::isExchangeCalibrated(const ExchangeId& exchange) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return exchangeParameters_.find(exchange) != exchangeParameters_.end();
}

std::chrono::system_clock::time_point ExchangeCalibration::getCalibrationTime(const ExchangeId& exchange) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = calibrationTimes_.find(exchange);
    if (it != calibrationTimes_.end()) {
        return it->second;
    }
    
    // Return epoch time if not calibrated
    return std::chrono::system_clock::time_point();
}

void ExchangeCalibration::setBaseParameters(const ParameterSet& baseParams) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    baseParameters_ = baseParams;
}

bool ExchangeCalibration::transferCalibration(const ExchangeId& source, const ExchangeId& target) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if source exchange is calibrated
    auto sourceParamsIt = exchangeParameters_.find(source);
    if (sourceParamsIt == exchangeParameters_.end()) {
        return false; // Source not calibrated
    }
    
    // Check similarity between exchanges
    auto sourceProfileIt = exchangeProfiles_.find(source);
    auto targetProfileIt = exchangeProfiles_.find(target);
    
    if (sourceProfileIt == exchangeProfiles_.end() || targetProfileIt == exchangeProfiles_.end()) {
        return false; // Missing exchange profiles
    }
    
    double similarity = calculateExchangeSimilarity(source, target);
    
    // Only transfer if exchanges are similar enough
    if (similarity < 0.7) {
        return false; // Not similar enough
    }
    
    // Transfer parameters with adjustments for differences
    ParameterSet targetParams = sourceParamsIt->second;
    
    // Apply adjustments based on differences between exchanges
    for (auto& param : targetParams) {
        // In a real implementation, this would be more sophisticated
        // Here we'll apply a simple scaling based on similarity
        param.second = param.second * (0.7 + 0.3 * similarity);
    }
    
    // Store calibrated parameters for target
    exchangeParameters_[target] = targetParams;
    
    // Update calibration timestamp
    calibrationTimes_[target] = std::chrono::system_clock::now();
    
    return true;
}

bool ExchangeCalibration::saveCalibration(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        nlohmann::json j;
        
        // Save base parameters
        j["baseParameters"] = nlohmann::json::object();
        for (const auto& param : baseParameters_) {
            j["baseParameters"][param.first] = param.second;
        }
        
        // Save exchange parameters
        j["exchangeParameters"] = nlohmann::json::object();
        for (const auto& exchange : exchangeParameters_) {
            std::string exchangeStr = std::to_string(static_cast<int>(exchange.first));
            j["exchangeParameters"][exchangeStr] = nlohmann::json::object();
            
            for (const auto& param : exchange.second) {
                j["exchangeParameters"][exchangeStr][param.first] = param.second;
            }
        }
        
        // Save calibration times
        j["calibrationTimes"] = nlohmann::json::object();
        for (const auto& time : calibrationTimes_) {
            std::string exchangeStr = std::to_string(static_cast<int>(time.first));
            // Convert time_point to time_t
            auto time_t_val = std::chrono::system_clock::to_time_t(time.second);
            j["calibrationTimes"][exchangeStr] = time_t_val;
        }
        
        // Write to file
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(4); // Pretty print with 4-space indent
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving calibration: " << e.what() << std::endl;
        return false;
    }
}

bool ExchangeCalibration::loadCalibration(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Read file
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        // Load base parameters
        baseParameters_.clear();
        if (j.contains("baseParameters") && j["baseParameters"].is_object()) {
            for (auto it = j["baseParameters"].begin(); it != j["baseParameters"].end(); ++it) {
                baseParameters_[it.key()] = it.value().get<double>();
            }
        }
        
        // Load exchange parameters
        exchangeParameters_.clear();
        if (j.contains("exchangeParameters") && j["exchangeParameters"].is_object()) {
            for (auto exchangeIt = j["exchangeParameters"].begin(); exchangeIt != j["exchangeParameters"].end(); ++exchangeIt) {
                ExchangeId exchangeId = static_cast<ExchangeId>(std::stoi(exchangeIt.key()));
                
                if (exchangeIt.value().is_object()) {
                    ParameterSet params;
                    for (auto paramIt = exchangeIt.value().begin(); paramIt != exchangeIt.value().end(); ++paramIt) {
                        params[paramIt.key()] = paramIt.value().get<double>();
                    }
                    exchangeParameters_[exchangeId] = params;
                }
            }
        }
        
        // Load calibration times
        calibrationTimes_.clear();
        if (j.contains("calibrationTimes") && j["calibrationTimes"].is_object()) {
            for (auto timeIt = j["calibrationTimes"].begin(); timeIt != j["calibrationTimes"].end(); ++timeIt) {
                ExchangeId exchangeId = static_cast<ExchangeId>(std::stoi(timeIt.key()));
                std::time_t time_t_val = timeIt.value().get<std::time_t>();
                calibrationTimes_[exchangeId] = std::chrono::system_clock::from_time_t(time_t_val);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading calibration: " << e.what() << std::endl;
        return false;
    }
}

double ExchangeCalibration::calculateExchangeSimilarity(const ExchangeId& exchange1, const ExchangeId& exchange2) const {
    auto it1 = exchangeProfiles_.find(exchange1);
    auto it2 = exchangeProfiles_.find(exchange2);
    
    if (it1 == exchangeProfiles_.end() || it2 == exchangeProfiles_.end()) {
        return 0.0; // No profiles, no similarity
    }
    
    const ExchangeMetrics& metrics1 = it1->second;
    const ExchangeMetrics& metrics2 = it2->second;
    
    // Calculate similarity based on key metrics
    // This is a simplified version - in reality this would be more sophisticated
    
    // Compare update frequency
    double freqSimilarity = 1.0 - std::min(1.0, std::abs(metrics1.updateFrequency - metrics2.updateFrequency) / metrics1.updateFrequency);
    
    // Compare spread patterns
    double spreadSimilarity = 1.0 - std::min(1.0, std::abs(metrics1.averageSpread - metrics2.averageSpread) / metrics1.averageSpread);
    
    // Compare order book depth
    double depthSimilarity = 1.0 - std::min(1.0, std::abs(metrics1.bookDepth - metrics2.bookDepth) / metrics1.bookDepth);
    
    // Compare liquidity
    double liquiditySimilarity = 1.0 - std::min(1.0, std::abs(metrics1.averageLiquidity - metrics2.averageLiquidity) / metrics1.averageLiquidity);
    
    // Calculate weighted average similarity
    return 0.3 * freqSimilarity + 0.3 * spreadSimilarity + 0.2 * depthSimilarity + 0.2 * liquiditySimilarity;
}

ExchangeFingerprint ExchangeCalibration::generateFingerprint(const ExchangeMetrics& metrics) const {
    ExchangeFingerprint fingerprint;
    
    // Create a fingerprint based on key characteristics
    fingerprint.updateFrequencyBucket = std::floor(metrics.updateFrequency / 10.0);  // Bucket by 10ms
    fingerprint.spreadBucket = std::floor(metrics.averageSpread * 1000.0);  // Bucket in basis points
    fingerprint.depthBucket = std::floor(metrics.bookDepth / 5.0);  // Bucket by groups of 5 levels
    fingerprint.liquidityBucket = std::floor(std::log10(metrics.averageLiquidity));  // Log scale buckets
    
    return fingerprint;
}

ParameterSet ExchangeCalibration::applyExchangeAdjustments(const ParameterSet& base, const ExchangeMetrics& metrics) const {
    ParameterSet adjusted = base;
    
    // Apply adjustments based on exchange characteristics
    // This is a simplified version - in reality this would be more sophisticated
    
    // Adjust imbalance threshold based on update frequency
    // More frequent updates may need higher thresholds to avoid noise
    if (adjusted.find("imbalanceThreshold") != adjusted.end()) {
        double frequencyFactor = std::sqrt(100.0 / std::max(1.0, metrics.updateFrequency));
        adjusted["imbalanceThreshold"] *= frequencyFactor;
    }
    
    // Adjust volume threshold based on exchange liquidity
    if (adjusted.find("volumeThreshold") != adjusted.end()) {
        double liquidityFactor = std::log10(metrics.averageLiquidity) / 6.0;  // Normalize to ~0.5-1.5 range
        adjusted["volumeThreshold"] *= liquidityFactor;
    }
    
    // Adjust signal duration based on market impact model
    if (adjusted.find("signalDuration") != adjusted.end()) {
        double impactFactor = 1.0 + (metrics.marketImpact - 0.5) * 0.5;  // Scale by market impact
        adjusted["signalDuration"] *= impactFactor;
    }
    
    // Adjust depth consideration based on book depth
    if (adjusted.find("considerationDepth") != adjusted.end()) {
        double depthFactor = std::min(1.5, std::max(0.5, metrics.bookDepth / 20.0));
        adjusted["considerationDepth"] *= depthFactor;
    }
    
    return adjusted;
}

} // namespace strategy
} // namespace trading