/**
 * @file strategy/types.hpp
 * @brief Combined types for the strategy component
 * 
 * This file aggregates all type definitions for the strategy component
 * to simplify imports.
 * 
 * @author Trading System Team
 * @date May 12, 2025
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
 #include "strategy/types/constants.hpp"
 
 namespace trading {
 namespace strategy {
 
 // Callback typedefs
 using ImbalanceCallback = std::function<void(const ImbalanceEvent&)>;
 using SignalCallback = std::function<void(const TradingSignal&)>;
 using MetricsCallback = std::function<void(const OrderBookMetrics&)>;
 
 // Forward declarations for important strategy classes
 class OrderBookAnalyzer;
 class ImbalanceDetectionEngine;
 class ImbalanceSignalGenerator;
 class StrategyEngine;
 
 // Shared pointers for strategy components
 using OrderBookAnalyzerPtr = std::shared_ptr<OrderBookAnalyzer>;
 using ImbalanceDetectionEnginePtr = std::shared_ptr<ImbalanceDetectionEngine>;
 using ImbalanceSignalGeneratorPtr = std::shared_ptr<ImbalanceSignalGenerator>;
 using StrategyEnginePtr = std::shared_ptr<StrategyEngine>;
 
 } // namespace strategy
 } // namespace trading