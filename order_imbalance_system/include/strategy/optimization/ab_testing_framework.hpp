// ab_testing_framework.hpp
#pragma once

#include "strategy/strategy_types.hpp"
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <random>

namespace trading {
namespace strategy {

/**
 * Class representing an experiment for A/B testing
 */
class Experiment {
public:
    Experiment(const std::string& id, const std::string& name);
    
    // Add a variant to the experiment
    void addVariant(const std::string& variantId, const StrategyVariant& variant, double trafficAllocation = 0.0);
    
    // Set traffic allocation for a variant
    void setTrafficAllocation(const std::string& variantId, double allocation);
    
    // Get information
    std::string getId() const { return id_; }
    std::string getName() const { return name_; }
    std::vector<std::string> getVariantIds() const;
    StrategyVariant getVariant(const std::string& variantId) const;
    double getTrafficAllocation(const std::string& variantId) const;
    
    // Status management
    bool isActive() const { return active_; }
    void setActive(bool active) { active_ = active; }
    
    // Duration management
    void setStartTime(std::chrono::system_clock::time_point time) { startTime_ = time; }
    void setEndTime(std::chrono::system_clock::time_point time) { endTime_ = time; }
    std::chrono::system_clock::time_point getStartTime() const { return startTime_; }
    std::chrono::system_clock::time_point getEndTime() const { return endTime_; }

private:
    std::string id_;
    std::string name_;
    bool active_ = false;
    std::chrono::system_clock::time_point startTime_;
    std::chrono::system_clock::time_point endTime_;
    std::map<std::string, StrategyVariant> variants_;
    std::map<std::string, double> trafficAllocations_;
};

/**
 * Class for A/B testing different strategy variants
 */
class ABTestingFramework {
public:
    // Constructor
    ABTestingFramework();
    ~ABTestingFramework() = default;
    
    // Define a new experiment
    void defineExperiment(const Experiment& experiment);
    
    // Get experiment by ID
    Experiment getExperiment(const std::string& experimentId) const;
    
    // Assign a variant for a specific order book
    StrategyVariant assignVariant(const std::string& experimentId, const OrderBookSnapshot& snapshot);
    
    // Record outcome for a variant
    void recordOutcome(const std::string& experimentId, const std::string& variantId, const PerformanceMetrics& metrics);
    
    // Get experiment results
    ExperimentResults getResults(const std::string& experimentId) const;
    
    // Start/stop experiments
    void startExperiment(const std::string& experimentId);
    void stopExperiment(const std::string& experimentId);
    
    // Multi-armed bandit optimization
    void enableAutomaticOptimization(const std::string& experimentId, bool enable);
    
    // Early stopping for underperforming variants
    void enableEarlyStopping(const std::string& experimentId, bool enable, double confidenceThreshold = 0.95);
    
private:
    // Active experiments
    std::map<std::string, Experiment> experiments_;
    
    // Experiment results
    std::map<std::string, ExperimentResults> results_;
    
    // Settings
    std::map<std::string, bool> automaticOptimizationEnabled_;
    std::map<std::string, bool> earlyStoppingEnabled_;
    std::map<std::string, double> earlyStoppingThresholds_;
    
    // Consistent hash function for variant assignment
    uint64_t consistentHash(const OrderBookSnapshot& snapshot) const;
    
    // Statistical significance testing
    bool isStatisticallySignificant(const ExperimentResults& results, double confidenceLevel) const;
    
    // Multi-armed bandit implementation
    void updateTrafficAllocation(const std::string& experimentId);
    
    // Random number generator for probabilistic variant assignment
    mutable std::mt19937 rng_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace strategy
} // namespace trading