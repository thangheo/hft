#pragma once

// #include "algorithms/twap_algorithm.hpp"
#include "execution/algorithms/twap_config.hpp"
#include "algorithms/vwap_algorithm.hpp"
#include "algorithms/iceberg_algorithm.hpp"
#include "algorithms/pov_algorithm.hpp"
#include "execution/execution_forward_decl.hpp"
#include "execution/execution_types.hpp"
#include "models/order.hpp"
#include <memory>
#include <string>
#include <unordered_map>
namespace trading {
namespace execution {

// Forward declarations
// class ExecutionService;
// struct ExecutionResponse;  
// struct DefaultAlgorithmParams;
// struct TwapParams;

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
 * Default algorithm parameters with basic trading information
 */
struct DefaultAlgorithmParams : public AlgorithmParams {
    std::string exchange_id;
    std::string trading_pair;
    models::OrderSide side;
    models::OrderType order_type;
    double quantity;
    double price;
    std::string client_order_id;
    models::TimeInForce time_in_force;
    std::string start_time; // Optional start time for execution
    std::string end_time;   // Optional end time for execution
    double visible_quantity; // Optional visible size for iceberg orders
    double duration_seconds;
    double participation_rate = 0.0; // For POV algorithm
    std::string signal_id;
    std::unordered_map<std::string, std::string> extra_params;
    std::chrono::system_clock::time_point created_at;
    AlgorithmType algorithm_type = AlgorithmType::TWAP; // Default algorithm
    
    AlgorithmType getType() const override { return algorithm_type; }
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
 * POV algorithm parameters
 */
struct PovParams : public AlgorithmParams {
    algorithms::PovConfig config;
    AlgorithmType getType() const override { return AlgorithmType::POV; }
};
/**
 * Base class for execution algorithms
 */
class ExecutionAlgorithm {
public:
    virtual ~ExecutionAlgorithm() = default;
    virtual ExecutionResponse execute(const models::Order& order, ExecutionService& execution_service) = 0;

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
    // std::shared_ptr<ExecutionAlgorithm> createAlgorithm(
    //     const std::string& id,
    //     const std::string& trading_pair,
    //     models::OrderSide side,
    //     const std::shared_ptr<AlgorithmParams>& params);
    std::shared_ptr<ExecutionAlgorithm> createAlgorithm(
        const std::shared_ptr<DefaultAlgorithmParams>& params) ;
    
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
    // Algorithm wrapper classes
    class TwapAlgorithmWrapper;
    class VwapAlgorithmWrapper;
    class IcebergAlgorithmWrapper;
    class PovAlgorithmWrapper;
    std::function<double(const std::string&, std::chrono::minutes)> volume_callback_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
    // Callbacks
    std::function<void(const models::OrderPtr&)> order_callback_;
    std::function<void(const std::string&, bool, const std::string&)> completion_callback_;
    
    // Algorithm wrapper classes
    class TwapAlgorithmWrapper;
    class VwapAlgorithmWrapper;
    class IcebergAlgorithmWrapper;
};

// Type aliases for convenience
using ExecutionAlgorithmPtr = std::shared_ptr<ExecutionAlgorithm>;
using AlgorithmParamsPtr = std::shared_ptr<AlgorithmParams>;

// 
// POV Algorithm Wrapper
class PovAlgorithmWrapper : public ExecutionAlgorithm {
    public:
        PovAlgorithmWrapper(
            const std::string& id,
            const std::string& trading_pair,
            models::OrderSide side,
            const algorithms::PovConfig& config)
            : id_(id), algo_() {
            
            // Initialize algorithm
            algo_.initialize(trading_pair, side, config);
        }
        
        bool start() override {
            return algo_.start();
        }
        
        bool pause() override {
            return algo_.pause();
        }
        
        bool resume() override {
            // POV doesn't have a resume method, use start instead if paused
            if (algo_.isRunning()) {
                return true; // Already running
            }
            return algo_.start();
        }
        
        bool cancel() override {
            return algo_.cancel();
        }
        
        bool isRunning() const override {
            return algo_.isRunning();
        }
        
        bool isComplete() const override {
            return algo_.isComplete();
        }
        
        AlgorithmType getType() const override {
            return AlgorithmType::POV;
        }
        
        std::string getId() const override {
            return id_;
        }
        
        std::vector<models::OrderPtr> getChildOrders() const override {
            return algo_.getChildOrders();
        }
        
        void handleOrderUpdate(const models::OrderPtr& order) override {
            algo_.handleOrderUpdate(order);
        }
        
        void setOrderCallback(const std::function<void(const models::OrderPtr&)>& callback) {
            algo_.setOrderCallback(callback);
        }
        
        void setCompletionCallback(
            const std::function<void(bool, const std::string&)>& callback) {
            algo_.setCompletionCallback(callback);
        }
        
        void setMarketVolumeCallback(
            const std::function<double(const std::string&, std::chrono::minutes)>& callback) {
            algo_.setMarketVolumeCallback(callback);
        }
        
    private:
        std::string id_;
        algorithms::PovAlgorithm algo_;
    };
} // namespace execution
} // namespace trading