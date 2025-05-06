// ab_testing_framework.cpp
#include "strategy/optimization/ab_testing_framework.hpp"
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>
#include <iostream>

namespace trading {
namespace strategy {

Experiment::Experiment(const std::string& id, const std::string& name)
    : id_(id), name_(name), active_(false) {
}

void Experiment::addVariant(const std::string& variantId, const StrategyVariant& variant, double trafficAllocation) {
    variants_[variantId] = variant;
    trafficAllocations_[variantId] = trafficAllocation;
}

void Experiment::setTrafficAllocation(const std::string& variantId, double allocation) {
    if (variants_.find(variantId) != variants_.end()) {
        trafficAllocations_[variantId] = allocation;
    }
}

std::vector<std::string> Experiment::getVariantIds() const {
    std::vector<std::string> ids;
    for (const auto& variant : variants_) {
        ids.push_back(variant.first);
    }
    return ids;
}

StrategyVariant Experiment::getVariant(const std::string& variantId) const {
    auto it = variants_.find(variantId);
    if (it != variants_.end()) {
        return it->second;
    }
    // Return empty variant if not found
    return StrategyVariant();
}

double Experiment::getTrafficAllocation(const std::string& variantId) const {
    auto it = trafficAllocations_.find(variantId);
    if (it != trafficAllocations_.end()) {
        return it->second;
    }
    return 0.0;
}

ABTestingFramework::ABTestingFramework() 
    : rng_(std::random_device()()) {
}

void ABTestingFramework::defineExperiment(const Experiment& experiment) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    experiments_[experiment.getId()] = experiment;
    
    // Initialize results
    results_[experiment.getId()] = ExperimentResults();
    results_[experiment.getId()].experimentId = experiment.getId();
    results_[experiment.getId()].experimentName = experiment.getName();
    
    for (const auto& variantId : experiment.getVariantIds()) {
        results_[experiment.getId()].variantResults[variantId] = VariantResult();
        results_[experiment.getId()].variantResults[variantId].variantId = variantId;
    }
    
    // Initialize settings
    automaticOptimizationEnabled_[experiment.getId()] = false;
    earlyStoppingEnabled_[experiment.getId()] = false;
    earlyStoppingThresholds_[experiment.getId()] = 0.95;
}

Experiment ABTestingFramework::getExperiment(const std::string& experimentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = experiments_.find(experimentId);
    if (it != experiments_.end()) {
        return it->second;
    }
    
    // Return empty experiment if not found
    return Experiment("", "");
}

StrategyVariant ABTestingFramework::assignVariant(const std::string& experimentId, const OrderBookSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto expIt = experiments_.find(experimentId);
    if (expIt == experiments_.end() || !expIt->second.isActive()) {
        // Return empty variant if experiment not found or not active
        return StrategyVariant();
    }
    
    Experiment& experiment = expIt->second;
    
    // Get variant IDs
    std::vector<std::string> variantIds = experiment.getVariantIds();
    if (variantIds.empty()) {
        return StrategyVariant();
    }
    
    // Decide which method to use for assignment
    bool useConsistentAssignment = true;  // Use consistent assignment for order book
    
    if (useConsistentAssignment) {
        // Use consistent hash for deterministic assignment
        uint64_t hash = consistentHash(snapshot);
        
        // Collect variants with their traffic allocations
        std::vector<std::pair<std::string, double>> variantsWithAllocation;
        double totalAllocation = 0.0;
        
        for (const auto& variantId : variantIds) {
            double allocation = experiment.getTrafficAllocation(variantId);
            variantsWithAllocation.push_back(std::make_pair(variantId, allocation));
            totalAllocation += allocation;
        }
        
        // Normalize allocations if they don't sum to 1.0
        if (totalAllocation > 0.0 && std::abs(totalAllocation - 1.0) > 0.001) {
            for (auto& variant : variantsWithAllocation) {
                variant.second /= totalAllocation;
            }
        }
        
        // Deterministically select variant based on hash
        double hashNormalized = (hash % 1000) / 1000.0;
        double cumulativeProbability = 0.0;
        
        for (const auto& variant : variantsWithAllocation) {
            cumulativeProbability += variant.second;
            if (hashNormalized < cumulativeProbability || 
                variant == variantsWithAllocation.back()) {
                // Record assignment in results
                auto& results = results_[experimentId];
                results.variantResults[variant.first].impressions++;
                
                // Return the selected variant
                return experiment.getVariant(variant.first);
            }
        }
    } else {
        // Probabilistic assignment using random number generator
        std::uniform_real_distribution<> dist(0.0, 1.0);
        double random = dist(rng_);
        
        double cumulativeProbability = 0.0;
        for (const auto& variantId : variantIds) {
            cumulativeProbability += experiment.getTrafficAllocation(variantId);
            if (random < cumulativeProbability || 
                variantId == variantIds.back()) {
                // Record assignment in results
                auto& results = results_[experimentId];
                results.variantResults[variantId].impressions++;
                
                // Return the selected variant
                return experiment.getVariant(variantId);
            }
        }
    }
    
    // Fallback: return first variant
    return experiment.getVariant(variantIds[0]);
}

void ABTestingFramework::recordOutcome(const std::string& experimentId, const std::string& variantId, const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if experiment exists
    auto expIt = results_.find(experimentId);
    if (expIt == results_.end()) {
        return;
    }
    
    // Check if variant exists
    auto varIt = expIt->second.variantResults.find(variantId);
    if (varIt == expIt->second.variantResults.end()) {
        return;
    }
    
    // Record metrics
    VariantResult& result = varIt->second;
    result.conversions++;
    
    // Update performance metrics
    result.metrics.push_back(metrics);
    
    // Calculate aggregate metrics
    result.aggregateMetrics.sharpeRatio = 0.0;
    result.aggregateMetrics.totalReturn = 0.0;
    result.aggregateMetrics.maxDrawdown = 0.0;
    result.aggregateMetrics.winRate = 0.0;
    
    for (const auto& m : result.metrics) {
        result.aggregateMetrics.sharpeRatio += m.sharpeRatio;
        result.aggregateMetrics.totalReturn += m.totalReturn;
        result.aggregateMetrics.maxDrawdown = std::max(result.aggregateMetrics.maxDrawdown, m.maxDrawdown);
        result.aggregateMetrics.winRate += m.winRate;
    }
    
    if (!result.metrics.empty()) {
        size_t count = result.metrics.size();
        result.aggregateMetrics.sharpeRatio /= count;
        result.aggregateMetrics.totalReturn /= count;
        result.aggregateMetrics.winRate /= count;
    }
    
    // Update conversion rate
    if (result.impressions > 0) {
        result.conversionRate = static_cast<double>(result.conversions) / result.impressions;
    }
    
    // Update experiment results with latest timestamp
    expIt->second.lastUpdateTime = std::chrono::system_clock::now();
    
    // If automatic optimization is enabled, update traffic allocations
    if (automaticOptimizationEnabled_[experimentId]) {
        updateTrafficAllocation(experimentId);
    }
    
    // Check for early stopping if enabled
    if (earlyStoppingEnabled_[experimentId] && 
        isStatisticallySignificant(expIt->second, earlyStoppingThresholds_[experimentId])) {
        // Find best variant
        std::string bestVariantId = expIt->second.variantResults.begin()->first;
        double bestSharpe = expIt->second.variantResults.begin()->second.aggregateMetrics.sharpeRatio;
        
        for (const auto& variant : expIt->second.variantResults) {
            if (variant.second.aggregateMetrics.sharpeRatio > bestSharpe) {
                bestVariantId = variant.first;
                bestSharpe = variant.second.aggregateMetrics.sharpeRatio;
            }
        }
        
        // Update experiment to use only the best variant
        auto expObjIt = experiments_.find(experimentId);
        if (expObjIt != experiments_.end()) {
            for (const auto& variantId : expObjIt->second.getVariantIds()) {
                if (variantId == bestVariantId) {
                    expObjIt->second.setTrafficAllocation(variantId, 1.0);
                } else {
                    expObjIt->second.setTrafficAllocation(variantId, 0.0);
                }
            }
        }
    }
}

ExperimentResults ABTestingFramework::getResults(const std::string& experimentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = results_.find(experimentId);
    if (it != results_.end()) {
        return it->second;
    }
    
    // Return empty results if not found
    ExperimentResults emptyResults;
    emptyResults.experimentId = experimentId;
    return emptyResults;
}

void ABTestingFramework::startExperiment(const std::string& experimentId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = experiments_.find(experimentId);
    if (it != experiments_.end()) {
        it->second.setActive(true);
        it->second.setStartTime(std::chrono::system_clock::now());
    }
}

void ABTestingFramework::stopExperiment(const std::string& experimentId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = experiments_.find(experimentId);
    if (it != experiments_.end()) {
        it->second.setActive(false);
        it->second.setEndTime(std::chrono::system_clock::now());
    }
}

void ABTestingFramework::enableAutomaticOptimization(const std::string& experimentId, bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    automaticOptimizationEnabled_[experimentId] = enable;
    
    // If enabling, initialize with equal traffic allocation
    if (enable) {
        auto it = experiments_.find(experimentId);
        if (it != experiments_.end()) {
            std::vector<std::string> variantIds = it->second.getVariantIds();
            double equalAllocation = 1.0 / variantIds.size();
            
            for (const auto& variantId : variantIds) {
                it->second.setTrafficAllocation(variantId, equalAllocation);
            }
        }
    }
}

void ABTestingFramework::enableEarlyStopping(const std::string& experimentId, bool enable, double confidenceThreshold) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    earlyStoppingEnabled_[experimentId] = enable;
    earlyStoppingThresholds_[experimentId] = confidenceThreshold;
}

uint64_t ABTestingFramework::consistentHash(const OrderBookSnapshot& snapshot) const {
    // Simple hash function for consistent assignment based on order book characteristics
    uint64_t hash = 0;
    
    // Hash based on exchange ID and symbol
    hash ^= std::hash<int>()(static_cast<int>(snapshot.exchange)) + 0x9e3779b9;
    hash ^= std::hash<std::string>()(snapshot.symbol) + 0x9e3779b9;
    
    // Hash based on some order book metrics
    if (!snapshot.bids.empty() && !snapshot.asks.empty()) {
        hash ^= std::hash<double>()(snapshot.bids[0].price) + 0x9e3779b9;
        hash ^= std::hash<double>()(snapshot.asks[0].price) + 0x9e3779b9;
        
        // Incorporate volumes at best levels
        if (!snapshot.bids.empty()) {
            hash ^= std::hash<double>()(snapshot.bids[0].quantity) + 0x9e3779b9;
        }
        if (!snapshot.asks.empty()) {
            hash ^= std::hash<double>()(snapshot.asks[0].quantity) + 0x9e3779b9;
        }
    }
    
    // Incorporate timestamp for time variability while maintaining consistency
    uint64_t timeBucket = snapshot.timestamp / (24 * 60 * 60 * 1000);  // Daily buckets
    hash ^= std::hash<uint64_t>()(timeBucket) + 0x9e3779b9;
    
    return hash;
}

bool ABTestingFramework::isStatisticallySignificant(const ExperimentResults& results, double confidenceLevel) const {
    // Check if we have at least two variants with enough data
    if (results.variantResults.size() < 2) {
        return false;
    }
    
    // Find the variant with the highest Sharpe ratio
    std::string bestVariantId;
    double bestSharpe = -std::numeric_limits<double>::infinity();
    
    for (const auto& variant : results.variantResults) {
        if (variant.second.aggregateMetrics.sharpeRatio > bestSharpe) {
            bestVariantId = variant.first;
            bestSharpe = variant.second.aggregateMetrics.sharpeRatio;
        }
    }
    
    // Check if the best variant has enough data
    const auto& bestVariant = results.variantResults.at(bestVariantId);
    if (bestVariant.metrics.size() < 30) {  // Require at least 30 data points
        return false;
    }
    
    // Calculate standard deviation of Sharpe ratio for best variant
    double sumSq = 0.0;
    for (const auto& metric : bestVariant.metrics) {
        double diff = metric.sharpeRatio - bestSharpe;
        sumSq += diff * diff;
    }
    double stdDevBest = std::sqrt(sumSq / bestVariant.metrics.size());
    
    // Check significance against each other variant
    for (const auto& variant : results.variantResults) {
        if (variant.first == bestVariantId) {
            continue;  // Skip comparing against itself
        }
        
        // Check if this variant has enough data
        if (variant.second.metrics.size() < 30) {
            return false;  // Not enough data
        }
        
        double otherSharpe = variant.second.aggregateMetrics.sharpeRatio;
        
        // Calculate standard deviation for this variant
        double otherSumSq = 0.0;
        for (const auto& metric : variant.second.metrics) {
            double diff = metric.sharpeRatio - otherSharpe;
            otherSumSq += diff * diff;
        }
        double stdDevOther = std::sqrt(otherSumSq / variant.second.metrics.size());
        
        // Calculate pooled standard error
        double stdError = std::sqrt(
            (stdDevBest * stdDevBest / bestVariant.metrics.size()) +
            (stdDevOther * stdDevOther / variant.second.metrics.size())
        );
        
        // Calculate z-score
        double zScore = (bestSharpe - otherSharpe) / stdError;
        
        // Check if significant at given confidence level
        // For 95% confidence, z > 1.96
        double zThreshold;
        if (confidenceLevel >= 0.99) {
            zThreshold = 2.576;
        } else if (confidenceLevel >= 0.95) {
            zThreshold = 1.96;
        } else if (confidenceLevel >= 0.90) {
            zThreshold = 1.645;
        } else {
            zThreshold = 1.28;  // 80% confidence
        }
        
        if (zScore < zThreshold) {
            return false;  // Not significant against this variant
        }
    }
    
    // Significant against all other variants
    return true;
}

void ABTestingFramework::updateTrafficAllocation(const std::string& experimentId) {
    // Multi-armed bandit implementation using Thompson sampling
    
    auto resultsIt = results_.find(experimentId);
    auto experimentIt = experiments_.find(experimentId);
    
    if (resultsIt == results_.end() || experimentIt == experiments_.end()) {
        return;
    }
    
    ExperimentResults& results = resultsIt->second;
    Experiment& experiment = experimentIt->second;
    
    // Get variant IDs
    std::vector<std::string> variantIds = experiment.getVariantIds();
    
    // Check if we have enough data
    bool enoughData = true;
    for (const auto& variantId : variantIds) {
        auto variantIt = results.variantResults.find(variantId);
        if (variantIt == results.variantResults.end() || variantIt->second.metrics.size() < 5) {
            enoughData = false;
            break;
        }
    }
    
    if (!enoughData) {
        // Not enough data, use equal allocation
        double equalAllocation = 1.0 / variantIds.size();
        for (const auto& variantId : variantIds) {
            experiment.setTrafficAllocation(variantId, equalAllocation);
        }
        return;
    }
    
    // Thompson sampling
    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::vector<double> samples;
    for (const auto& variantId : variantIds) {
        const auto& variant = results.variantResults.at(variantId);
        
        // Calculate mean and standard deviation of Sharpe ratio
        double mean = variant.aggregateMetrics.sharpeRatio;
        
        double sumSq = 0.0;
        for (const auto& metric : variant.metrics) {
            double diff = metric.sharpeRatio - mean;
            sumSq += diff * diff;
        }
        double stdDev = std::sqrt(sumSq / variant.metrics.size());
        
        // Sample from normal distribution
        std::normal_distribution<> dist(mean, stdDev + 0.0001);  // Add small constant to avoid division by zero
        samples.push_back(dist(gen));
    }
    
    // Find best sample
    int bestIdx = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        if (samples[i] > samples[bestIdx]) {
            bestIdx = i;
        }
    }
    
    // Allocate more traffic to the best variant, but keep some exploration
    for (size_t i = 0; i < variantIds.size(); ++i) {
        if (i == bestIdx) {
            experiment.setTrafficAllocation(variantIds[i], 0.8);  // 80% to best
        } else {
            // Distribute remaining 20% equally among others
            experiment.setTrafficAllocation(variantIds[i], 0.2 / (variantIds.size() - 1));
        }
    }
}

} // namespace strategy
} // namespace trading