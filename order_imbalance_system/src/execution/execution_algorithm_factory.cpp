// execution_algorithm_factory.cpp
#include "../include/execution/execution_algorithm_factory.hpp"
#include <chrono>
#include <iostream>
#include <uuid/uuid.h>

namespace trading {
namespace execution {

// Generate a unique ID for algorithms
std::string generateUniqueId() {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

// Wrapper classes for algorithm implementations

// TWAP Algorithm Wrapper
class ExecutionAlgorithmFactory::TwapAlgorithmWrapper : public ExecutionAlgorithm {
public:
    TwapAlgorithmWrapper(
        const std::string& id,
        const std::string& trading_pair,
        models::OrderSide side,
        const algorithms::TwapConfig& config)
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
        return algo_.resume();
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
        return AlgorithmType::TWAP;
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
    
private:
    std::string id_;
    algorithms::TwapAlgorithm algo_;
};

// VWAP Algorithm Wrapper
class ExecutionAlgorithmFactory::VwapAlgorithmWrapper : public ExecutionAlgorithm {
public:
    VwapAlgorithmWrapper(
        const std::string& id,
        const std::string& trading_pair,
        models::OrderSide side,
        const algorithms::VwapConfig& config)
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
        return algo_.resume();
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
        return AlgorithmType::VWAP;
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
    
    void setVolumeCallback(
        const std::function<double(const std::string&, std::chrono::minutes)>& callback) {
        algo_.setVolumeCallback(callback);
    }
    
private:
    std::string id_;
    algorithms::VwapAlgorithm algo_;
};

// Iceberg Algorithm Wrapper
class ExecutionAlgorithmFactory::IcebergAlgorithmWrapper : public ExecutionAlgorithm {
public:
    IcebergAlgorithmWrapper(
        const std::string& id,
        const std::string& trading_pair,
        models::OrderSide side,
        const algorithms::IcebergConfig& config)
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
        return algo_.resume();
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
        return AlgorithmType::ICEBERG;
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
    
private:
    std::string id_;
    algorithms::IcebergAlgorithm algo_;
};

ExecutionAlgorithmFactory::ExecutionAlgorithmFactory() = default;


// std::shared_ptr<ExecutionAlgorithm> ExecutionAlgorithmFactory::createAlgorithm(
//     const std::shared_ptr<DefaultAlgorithmParams>& params) {
    
//     std::lock_guard<std::mutex> lock(mutex_);
    
//     if (!params) {
//         std::cerr << "Algorithm parameters cannot be null" << std::endl;
//         return nullptr;
//     }
    
//     // Generate a unique ID
//     std::string algorithm_id = generateUniqueId();
    
//     // Create algorithm based on type
//     std::shared_ptr<ExecutionAlgorithm> algorithm;
    
//     switch (params->getType()) {
//         case AlgorithmType::TWAP: {
//             auto twap_params = std::dynamic_pointer_cast<TwapParams>(params);
//             if (!twap_params) {
//                 std::cerr << "Invalid TWAP parameters" << std::endl;
//                 return nullptr;
//             }
            
//             auto wrapper = std::make_shared<TwapAlgorithmWrapper>(
//                 algorithm_id, twap_params->trading_pair, twap_params->side, twap_params->config);
            
//             if (order_callback_) {
//                 wrapper->setOrderCallback(order_callback_);
//             }
            
//             if (completion_callback_) {
//                 wrapper->setCompletionCallback(
//                     [this, algorithm_id](bool success, const std::string& message) {
//                         completion_callback_(algorithm_id, success, message);
//                     });
//             }
            
//             algorithm = wrapper;
//             break;
//         }
        
//         case AlgorithmType::VWAP: {
//             auto vwap_params = std::dynamic_pointer_cast<VwapParams>(params);
//             if (!vwap_params) {
//                 std::cerr << "Invalid VWAP parameters" << std::endl;
//                 return nullptr;
//             }
            
//             auto wrapper = std::make_shared<VwapAlgorithmWrapper>(
//                 algorithm_id, vwap_params->trading_pair, vwap_params->side, vwap_params->config);
            
//             if (order_callback_) {
//                 wrapper->setOrderCallback(order_callback_);
//             }
            
//             if (completion_callback_) {
//                 wrapper->setCompletionCallback(
//                     [this, algorithm_id](bool success, const std::string& message) {
//                         completion_callback_(algorithm_id, success, message);
//                     });
//             }
            
//             if (volume_callback_) {
//                 wrapper->setVolumeCallback(volume_callback_);
//             }
            
//             algorithm = wrapper;
//             break;
//         }
        
//         case AlgorithmType::ICEBERG: {
//             auto iceberg_params = std::dynamic_pointer_cast<IcebergParams>(params);
//             if (!iceberg_params) {
//                 std::cerr << "Invalid Iceberg parameters" << std::endl;
//                 return nullptr;
//             }
            
//             auto wrapper = std::make_shared<IcebergAlgorithmWrapper>(
//                 algorithm_id, iceberg_params->trading_pair, iceberg_params->side, iceberg_params->config);
            
//             if (order_callback_) {
//                 wrapper->setOrderCallback(order_callback_);
//             }
            
//             if (completion_callback_) {
//                 wrapper->setCompletionCallback(
//                     [this, algorithm_id](bool success, const std::string& message) {
//                         completion_callback_(algorithm_id, success, message);
//                     });
//             }
            
//             algorithm = wrapper;
//             break;
//         }
        
//         case AlgorithmType::POV: {
//             auto pov_params = std::dynamic_pointer_cast<PovParams>(params);
//             if (!pov_params) {
//                 std::cerr << "Invalid POV parameters" << std::endl;
//                 return nullptr;
//             }
            
//             auto wrapper = std::make_shared<PovAlgorithmWrapper>(
//                 algorithm_id, pov_params->trading_pair, pov_params->side, pov_params->config);
            
//             if (order_callback_) {
//                 wrapper->setOrderCallback(order_callback_);
//             }
            
//             if (completion_callback_) {
//                 wrapper->setCompletionCallback(
//                     [this, algorithm_id](bool success, const std::string& message) {
//                         completion_callback_(algorithm_id, success, message);
//                     });
//             }
            
//             if (volume_callback_) {
//                 wrapper->setVolumeCallback(volume_callback_);
//             }
            
//             algorithm = wrapper;
//             break;
//         }
        
//         default:
//             std::cerr << "Unsupported algorithm type: " << static_cast<int>(params->getType()) << std::endl;
//             return nullptr;
//     }
    
//     // Store algorithm in the factory's map
//     algorithms_[algorithm_id] = algorithm;
    
//     return algorithm;
// }

std::shared_ptr<ExecutionAlgorithm> ExecutionAlgorithmFactory::createAlgorithm(
    const std::shared_ptr<DefaultAlgorithmParams>& params) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!params) {
        std::cerr << "Algorithm parameters cannot be null" << std::endl;
        return nullptr;
    }
    
    // Generate a unique ID
    std::string algorithm_id = generateUniqueId();
    
    // Create algorithm based on type
    std::shared_ptr<ExecutionAlgorithm> algorithm;
    
    switch (params->getType()) {
        case AlgorithmType::TWAP: {
            // Create TWAP config from default params
            algorithms::TwapConfig twap_config;
            twap_config.total_quantity = params->quantity;
            twap_config.duration = std::chrono::seconds(static_cast<int>(params->duration_seconds));
            twap_config.slice_interval = std::chrono::seconds(static_cast<int>(params->duration_seconds / 10)); // Default to 10 slices
            twap_config.price_limit = params->price;
            
            auto wrapper = std::make_shared<TwapAlgorithmWrapper>(
                algorithm_id, params->trading_pair, params->side, twap_config);
            
            if (order_callback_) {
                wrapper->setOrderCallback(order_callback_);
            }
            
            if (completion_callback_) {
                wrapper->setCompletionCallback(
                    [this, algorithm_id](bool success, const std::string& message) {
                        completion_callback_(algorithm_id, success, message);
                    });
            }
            
            algorithm = wrapper;
            break;
        }
        
        case AlgorithmType::VWAP: {
            // Create VWAP config from default params
            algorithms::VwapConfig vwap_config;
            vwap_config.total_quantity = params->quantity;
            vwap_config.duration = std::chrono::seconds(static_cast<int>(params->duration_seconds));
            vwap_config.price_limit = params->price;
            
            // Default volume profile (equal distribution)
            std::vector<double> volume_profile(24, 1.0/24.0);
            vwap_config.volume_profile = volume_profile;
            
            auto wrapper = std::make_shared<VwapAlgorithmWrapper>(
                algorithm_id, params->trading_pair, params->side, vwap_config);
            
            if (order_callback_) {
                wrapper->setOrderCallback(order_callback_);
            }
            
            if (completion_callback_) {
                wrapper->setCompletionCallback(
                    [this, algorithm_id](bool success, const std::string& message) {
                        completion_callback_(algorithm_id, success, message);
                    });
            }
            
            if (volume_callback_) {
                wrapper->setVolumeCallback(volume_callback_);
            }
            
            algorithm = wrapper;
            break;
        }
        
        case AlgorithmType::ICEBERG: {
            // Create Iceberg config from default params
            algorithms::IcebergConfig iceberg_config;
            iceberg_config.total_quantity = params->quantity;
            iceberg_config.visible_quantity = params->visible_quantity > 0 ? 
                params->visible_quantity : params->quantity / 10.0; // Default to 1/10th
            iceberg_config.price = params->price;
            
            auto wrapper = std::make_shared<IcebergAlgorithmWrapper>(
                algorithm_id, params->trading_pair, params->side, iceberg_config);
            
            if (order_callback_) {
                wrapper->setOrderCallback(order_callback_);
            }
            
            if (completion_callback_) {
                wrapper->setCompletionCallback(
                    [this, algorithm_id](bool success, const std::string& message) {
                        completion_callback_(algorithm_id, success, message);
                    });
            }
            
            algorithm = wrapper;
            break;
        }
        
        default:
            std::cerr << "Unsupported algorithm type: " << static_cast<int>(params->getType()) << std::endl;
            return nullptr;
    }
    
    // Store algorithm in the factory's map
    algorithms_[algorithm_id] = algorithm;
    
    return algorithm;
}

std::shared_ptr<ExecutionAlgorithm> ExecutionAlgorithmFactory::getAlgorithm(
    const std::string& id) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = algorithms_.find(id);
    if (it != algorithms_.end()) {
        return it->second;
    }
    
    return nullptr;
}

void ExecutionAlgorithmFactory::registerOrderCallback(
    const std::function<void(const models::OrderPtr&)>& callback) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    order_callback_ = callback;
    
    // Update callback for existing algorithms
    for (auto& pair : algorithms_) {
        if (pair.second->getType() == AlgorithmType::TWAP) {
            auto wrapper = std::dynamic_pointer_cast<TwapAlgorithmWrapper>(pair.second);
            if (wrapper) {
                wrapper->setOrderCallback(callback);
            }
        } else if (pair.second->getType() == AlgorithmType::VWAP) {
            auto wrapper = std::dynamic_pointer_cast<VwapAlgorithmWrapper>(pair.second);
            if (wrapper) {
                wrapper->setOrderCallback(callback);
            }
        } else if (pair.second->getType() == AlgorithmType::ICEBERG) {
            auto wrapper = std::dynamic_pointer_cast<IcebergAlgorithmWrapper>(pair.second);
            if (wrapper) {
                wrapper->setOrderCallback(callback);
            }
        }
    }
}

void ExecutionAlgorithmFactory::registerCompletionCallback(
    const std::function<void(const std::string&, bool, const std::string&)>& callback) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    completion_callback_ = callback;
    
    // Update callback for existing algorithms
    for (auto& pair : algorithms_) {
        if (pair.second->getType() == AlgorithmType::TWAP) {
            auto wrapper = std::dynamic_pointer_cast<TwapAlgorithmWrapper>(pair.second);
            if (wrapper) {
                wrapper->setCompletionCallback(
                    [this, id = pair.first](bool success, const std::string& message) {
                        completion_callback_(id, success, message);
                    });
            }
        } else if (pair.second->getType() == AlgorithmType::VWAP) {
            auto wrapper = std::dynamic_pointer_cast<VwapAlgorithmWrapper>(pair.second);
            if (wrapper) {
                wrapper->setCompletionCallback(
                    [this, id = pair.first](bool success, const std::string& message) {
                        completion_callback_(id, success, message);
                    });
            }
        } else if (pair.second->getType() == AlgorithmType::ICEBERG) {
            auto wrapper = std::dynamic_pointer_cast<IcebergAlgorithmWrapper>(pair.second);
            if (wrapper) {
                wrapper->setCompletionCallback(
                    [this, id = pair.first](bool success, const std::string& message) {
                        completion_callback_(id, success, message);
                    });
            }
        }
    }
}

void ExecutionAlgorithmFactory::registerVolumeCallback(
    const std::function<double(const std::string&, std::chrono::minutes)>& callback) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    volume_callback_ = callback;
    
    // Update callback for existing VWAP algorithms
    for (auto& pair : algorithms_) {
        if (pair.second->getType() == AlgorithmType::VWAP) {
            auto wrapper = std::dynamic_pointer_cast<VwapAlgorithmWrapper>(pair.second);
            if (wrapper) {
                wrapper->setVolumeCallback(callback);
            }
        }
    }
}

} // namespace execution
} // namespace trading