/**
 * @file strategy/types.hpp
 * @brief Combined types for the strategy component
 * 
 * This file aggregates all type definitions for the strategy component
 * to simplify imports.
 * 
 * @author Trading System Team
 * @date May 13, 2025
 */

 #pragma once

 // Include core types
 #include "core/types.hpp"
 #include "core/time.hpp"
 #include "core/direction.hpp"
 #include "core/confidence.hpp"
 #include "core/market_types.hpp"
 
 // Include strategy-specific types
 #include "strategy/types/imbalance_type.hpp"
 #include "strategy/types/imbalance_threshold.hpp"
 #include "strategy/types/imbalance_event.hpp"
 #include "strategy/types/trading_signal.hpp"
 #include "strategy/types/parameter_set.hpp"
 #include "strategy/types/order_book_metrics.hpp"
 #include "strategy/types/strategy_performance.hpp"
 #include "strategy/types/component_types.hpp"
 #include "strategy/types/constants.hpp"
 #include "strategy/types/config_types.hpp"
 namespace trading {
 namespace strategy {
 
 // Callback typedefs
 using ImbalanceCallback = std::function<void(const ImbalanceEvent&)>;
 using SignalCallback = std::function<void(const TradingSignal&)>;
 using MetricsCallback = std::function<void(const OrderBookMetrics&)>;
 
 // Additional type aliases
 struct PriceMovement {
     core::Symbol symbol;
     core::Timestamp timestamp;
     double price;
     double previousPrice;
 };
 
 } // namespace strategy
 } // namespace trading