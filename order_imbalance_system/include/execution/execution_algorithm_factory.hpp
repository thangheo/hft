#pragma once

#include "algorithms/twap_algorithm.hpp"
#include "algorithms/vwap_algorithm.hpp"
#include "algorithms/iceberg_algorithm.hpp"
#include "models/order.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace trading {
namespace execution {

/**
 * Enumeration of supported execution algorithm types
 */
enum class AlgorithmType {
    TWAP,
    VWAP,
    ICEBERG,
    POV,         // Percentage of Volume
    IMPLEMENTATION_SHORTFALL,
    ADAPTIVE
};

/**
 * Base class for algorithm parameters
 */
struct AlgorithmParams {
    virtual ~AlgorithmParams() = default;
    virtual AlgorithmType getType() const = 0;
};

/**
 * TWAP algorithm parameters
 */
struct TwapParams : public AlgorithmParams {
    algorithms::TwapConfig config;
    AlgorithmType getType() const override { return AlgorithmType::TWAP; }
};

/**
 * VWAP algorithm parameters
 */
struct VwapParams : public AlgorithmParams {
    algorithms::VwapConfig config;
    AlgorithmType getType() const override { return AlgorithmType::VWAP; }
};

/**
 * Iceberg algorithm parameters
 */
struct IcebergParams : public AlgorithmParams {
    algorithms::IcebergConfig config;
    AlgorithmType getType() const override { return AlgorithmType::ICEBERG; }
};

/**
 * Base class for execution algorithms
 */
class ExecutionAlgorithm {
public:
    virtual ~ExecutionAlgorithm() = default;
    
    // Common interface methods
    virtual bool start() = 0;
    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual bool cancel() = 0;
    virtual bool isRunning() const = 0;
    virtual bool isComplete() const = 0;
    virtual AlgorithmType getType() const = 0;
    virtual std::string getId() const = 0;
    virtual std::vector<models::OrderPtr> getChildOrders() const = 0;
    virtual void handleOrderUpdate(const models::OrderPtr& order) = 0;
};

/**
 * Factory for creating execution algorithms
 */
class ExecutionAlgorithmFactory {
public:
    // Constructor
    ExecutionAlgorithmFactory();
    
    // Create algorithm instance
    std::shared_ptr<ExecutionAlgorithm> createAlgorithm(
        const std::string& id,
        const std::string& trading_pair,
        models::OrderSide side,
        const std::shared_ptr<AlgorithmParams>& params);
    
    // Get algorithm by ID
    std::shared_ptr<ExecutionAlgorithm> getAlgorithm(const std::string& id) const;
    
    // Register callbacks for all algorithms
    void registerOrderCallback(
        const std::function<void(const models::OrderPtr&)>& callback);
    
    void registerCompletionCallback(
        const std::function<void(const std::string&, bool, const std::string&)>& callback);
    
    // Register volume callback for VWAP algorithms
    void registerVolumeCallback(
        const std::function<double(const std::string&, std::chrono::minutes)>& callback);
    
private:
    std::unordered_map<std::string, std::shared_ptr<ExecutionAlgorithm>> algorithms_;
    
    // Callback storage
    std::function<void(const models::OrderPtr&)> order_callback_;
    std::function<void(const std::string&, bool, const std::string&)> completion_callback_;
    std::function<double(const std::string&, std::chrono::minutes)> volume_callback_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
    
    // Algorithm wrapper classes
    class TwapAlgorithmWrapper;
    class VwapAlgorithmWrapper;
    class IcebergAlgorithmWrapper;
};

// Type aliases for convenience
using ExecutionAlgorithmPtr = std::shared_ptr<ExecutionAlgorithm>;
using AlgorithmParamsPtr = std::shared_ptr<AlgorithmParams>;

} // namespace execution
} // namespace trading