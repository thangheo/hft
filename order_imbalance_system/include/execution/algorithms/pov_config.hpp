#pragma once

#include <string>
#include <chrono>

namespace trading {
namespace execution {
namespace algorithms {

/**
 * Configuration parameters for POV algorithm
 */
struct PovConfig {
    double total_quantity;               // Total quantity to execute
    std::chrono::seconds duration;       // Maximum duration of execution
    double participation_rate;           // Target percentage of market volume (0.01-1.0)
    double min_order_size = 0.0;         // Minimum size for each child order
    double max_order_size = 0.0;         // Maximum size for each child order
    std::chrono::seconds check_interval = std::chrono::seconds(30); // Interval for checking market volume
    double price_limit = 0.0;            // Optional price limit, 0 for market orders
    double deviation_pct = 0.01;         // Max allowed deviation from target price (1%)
    bool dynamic_rate = false;           // Whether to adjust participation rate dynamically
    std::chrono::minutes volume_lookback = std::chrono::minutes(5); // Period to look back for volume analysis
    std::string exchange_id;             // Target exchange for execution
    double urgency_factor = 0.5;         // 0-1 factor for urgency (affects participation rate)
};

} // namespace algorithms
} // namespace execution
} // namespace trading