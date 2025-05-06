/**
 * @file imbalance_signal_generator.cpp
 * @brief Implementation of trading signal generation from order book imbalances
 */

 #include "strategy/signal/imbalance_signal_generator.hpp"
 #include "strategy/signal/momentum_correlation.hpp"
 #include "strategy/signal/threshold_manager.hpp"
 #include "strategy/signal/signal_decay_model.hpp"
 #include <algorithm>
 #include <chrono>
 #include <cmath>
 #include <sstream>
 #include <random>
 
//  using namespace trading::strategy;
 namespace trading {
 namespace strategy {
 
 ImbalanceSignalGenerator::ImbalanceSignalGenerator() {
     // Initialize with default signal parameters
     signal_params_.min_strength_threshold = 0.3;
     signal_params_.min_confidence_threshold = 0.4;
     signal_params_.conflict_resolution_strategy = ConflictResolutionStrategy::STRENGTH_WEIGHTED;
     signal_params_.default_ttl_ms = 5000; // 5 seconds default TTL
 }
 
 void ImbalanceSignalGenerator::processImbalances(const std::vector<ImbalanceEvent>& imbalances) {
     if (imbalances.empty()) {
         return;
     }
     
     // Add new imbalances to pending processing queue
     pending_imbalances_.insert(pending_imbalances_.end(), imbalances.begin(), imbalances.end());
     
     // Optionally limit the queue size to prevent memory growth
     constexpr size_t MAX_PENDING_EVENTS = 1000;
     if (pending_imbalances_.size() > MAX_PENDING_EVENTS) {
         pending_imbalances_.erase(pending_imbalances_.begin(), 
                                   pending_imbalances_.begin() + 
                                   (pending_imbalances_.size() - MAX_PENDING_EVENTS));
     }
 }
 
 std::vector<TradingSignal> ImbalanceSignalGenerator::generateSignals() {
     if (pending_imbalances_.empty()) {
         return {};
     }
     
     std::vector<TradingSignal> signals;
     signals.reserve(pending_imbalances_.size());
     
     // Convert each imbalance event to a trading signal
     for (const auto& event : pending_imbalances_) {
         // Skip weak imbalances
         if (event.strength < signal_params_.min_strength_threshold) {
             continue;
         }
         
         TradingSignal signal = createSignalFromImbalance(event);
         
         // Apply minimum confidence threshold
         if (signal.confidence >= signal_params_.min_confidence_threshold) {
             signals.push_back(std::move(signal));
         }
     }
     
     // Clear pending imbalances after processing
     pending_imbalances_.clear();
     
     // Resolve any conflicting signals
     signals = resolveSignalConflicts(signals);
     
     // Notify callback if registered
     if (!signals.empty() && signal_callback_) {
         signal_callback_(signals);
     }
     
     return signals;
 }
 
 void ImbalanceSignalGenerator::configureSignalParameters(const SignalParameters& params) {
     signal_params_ = params;
 }
 
 void ImbalanceSignalGenerator::registerSignalCallback(const SignalCallback& callback) {
     signal_callback_ = callback;
 }
 
 void ImbalanceSignalGenerator::clear() {
     pending_imbalances_.clear();
 }
 
 void ImbalanceSignalGenerator::setMomentumCorrelation(std::shared_ptr<MomentumCorrelation> momentum_correlation) {
     momentum_correlation_ = momentum_correlation;
 }
 
 void ImbalanceSignalGenerator::setThresholdManager(std::shared_ptr<ThresholdManager> threshold_manager) {
     threshold_manager_ = threshold_manager;
 }
 
 void ImbalanceSignalGenerator::setSignalDecayModel(std::shared_ptr<SignalDecayModel> signal_decay_model) {
     signal_decay_model_ = signal_decay_model;
 }
 
 TradingSignal ImbalanceSignalGenerator::createSignalFromImbalance(const ImbalanceEvent& event) {
     TradingSignal signal;
     
     // Generate a unique signal ID using a combination of timestamp and event ID
     std::stringstream ss;
     ss << "SIG-" << std::chrono::system_clock::now().time_since_epoch().count() 
        << "-" << event.eventId;
     signal.id = ss.str();
     
     // Set basic signal properties from event
     signal.direction = event.direction;
     signal.sourceType = event.type;
     signal.generationTime = std::chrono::system_clock::now().time_since_epoch().count();
     
     // Calculate signal strength based on imbalance metrics
     signal.strength = calculateSignalStrength(event);
     
     // Calculate confidence level
     signal.confidence = calculateConfidence(event);
     
     // Determine signal time-to-live
     signal.ttl = determineSignalTTL(event);
     
     // Add signal metrics
     signal.signalMetrics = event.supportingMetrics;
     signal.signalMetrics["primary_metric"] = event.primaryMetric;
     signal.signalMetrics["detection_time_ms"] = static_cast<double>(
         event.detectionTime - signal.generationTime);
     
     // Add price targets if momentum correlation is available
     if (momentum_correlation_) {
         double expectedMove = momentum_correlation_->predictPriceMovement(event);
         if (std::abs(expectedMove) > 0.0001) {
             PriceTarget target;
             target.price = expectedMove;
             target.confidence = signal.confidence * 0.8; // Slightly lower confidence for the price target
             target.timeframeMs = signal.ttl;
             signal.targets.push_back(target);
         }
     }
     
     return signal;
 }
 
 double ImbalanceSignalGenerator::calculateSignalStrength(const ImbalanceEvent& event) {
     // Start with the raw imbalance strength
     double strength = event.strength;
     
     // Apply momentum correlation boost if available
     if (momentum_correlation_) {
         // Get correlation coefficient for this type of imbalance
         double correlation = momentum_correlation_->getCorrelationCoefficient(
             ImbalanceTypeToString(event.type));
         
         // Boost or reduce strength based on correlation
         strength *= (1.0 + std::abs(correlation) * 0.5);
         
         // If correlation is negative and contradicts event direction, reduce strength
         if ((correlation < 0 && event.direction == Direction::BUY) ||
             (correlation > 0 && event.direction == Direction::SELL)) {
             strength *= 0.5;
         }
     }
     
     // Apply threshold manager adjustments if available
     if (threshold_manager_) {
         ThresholdSet thresholds = threshold_manager_->getCurrentThresholds();
         
         // Find threshold for this imbalance type
         auto it = thresholds.typeThresholds.find(event.type);
         if (it != thresholds.typeThresholds.end()) {
             // Scale strength relative to the threshold
             strength = std::max(0.0, strength - it->second.min_threshold) / 
                       (it->second.max_threshold - it->second.min_threshold);
         }
     }
     
     // Clamp strength to valid range
     return std::max(0.0, std::min(1.0, strength));
 }
 
 double ImbalanceSignalGenerator::calculateConfidence(const ImbalanceEvent& event) {
     // Start with event confidence
     double confidence = event.confidence;
     
     // Adjust based on recent performance if available in signal parameters
     auto it = signal_params_.typePerformance.find(event.type);
     if (it != signal_params_.typePerformance.end()) {
         const auto& performance = it->second;
         
         // Adjust confidence based on historical accuracy
         confidence *= (0.5 + 0.5 * performance.accuracy);
         
         // Reduce confidence for signals that have been inconsistent
         confidence *= (1.0 - 0.3 * performance.volatility);
     }
     
     // Adjust confidence based on market regime if momentum correlation available
     if (momentum_correlation_) {
         MarketRegime regime = momentum_correlation_->detectMarketRegime();
         
         // Reduce confidence in high volatility regimes
         if (regime == MarketRegime::HIGH_VOLATILITY) {
             confidence *= 0.8;
         }
         // Reduce confidence in range-bound markets for trend signals
         else if (regime == MarketRegime::RANGE_BOUND && 
                 (event.type == ImbalanceType::TREND_CONTINUATION || 
                  event.type == ImbalanceType::TREND_REVERSAL)) {
             confidence *= 0.7;
         }
     }
     
     // Clamp confidence to valid range
     return std::max(0.0, std::min(1.0, confidence));
 }
 
 uint64_t ImbalanceSignalGenerator::determineSignalTTL(const ImbalanceEvent& event) {
     // Start with default TTL from parameters
     uint64_t ttl = signal_params_.default_ttl_ms;
     
     // Adjust based on imbalance type
     switch (event.type) {
         case ImbalanceType::VOLUME_IMBALANCE:
             // Volume imbalances tend to resolve more quickly
             ttl = static_cast<uint64_t>(ttl * 0.8);
             break;
         case ImbalanceType::LIQUIDITY_IMBALANCE:
             // Liquidity imbalances can persist longer
             ttl = static_cast<uint64_t>(ttl * 1.2);
             break;
         case ImbalanceType::PRICE_LEVEL_IMBALANCE:
             // Price level imbalances have average persistence
             break;
         case ImbalanceType::SPREAD_ANOMALY:
             // Spread anomalies can resolve very quickly
             ttl = static_cast<uint64_t>(ttl * 0.5);
             break;
         case ImbalanceType::TREND_CONTINUATION:
         case ImbalanceType::TREND_REVERSAL:
             // Trend signals may last longer
             ttl = static_cast<uint64_t>(ttl * 1.5);
             break;
         default:
             // Use default TTL for other types
             break;
     }
     
     // Scale TTL by signal strength - stronger signals may persist longer
     ttl = static_cast<uint64_t>(ttl * (0.8 + 0.4 * event.strength));
     
     // If signal decay model is available, use it to adjust TTL
     if (signal_decay_model_) {
         // Create a temporary signal to evaluate decay
         TradingSignal tempSignal;
         tempSignal.sourceType = event.type;
         tempSignal.strength = event.strength;
         
         // Get decay model type
         DecayModelType modelType = signal_decay_model_->getModelType();
         
         // Adjust TTL based on decay model type
         if (modelType == DecayModelType::EXPONENTIAL) {
             // Exponential decay means the signal becomes insignificant more quickly
             ttl = static_cast<uint64_t>(ttl * 0.9);
         } else if (modelType == DecayModelType::STEP) {
             // Step decay can extend effective TTL
             ttl = static_cast<uint64_t>(ttl * 1.1);
         }
     }
     
     return ttl;
 }
 
 std::vector<TradingSignal> ImbalanceSignalGenerator::resolveSignalConflicts(
     const std::vector<TradingSignal>& signals) {
     
     if (signals.size() <= 1) {
         return signals;
     }
     
     // Group signals by trading pair
     std::map<std::string, std::vector<TradingSignal>> signalsByPair;
     for (const auto& signal : signals) {
         // Extract trading pair from signal metrics or ID
         std::string tradingPair = "UNKNOWN";
         auto it = signal.signalMetrics.find("trading_pair");
         if (it != signal.signalMetrics.end()) {
             tradingPair = it->second;
         }
         signalsByPair[tradingPair].push_back(signal);
     }
     
     std::vector<TradingSignal> resolvedSignals;
     
     // Process each group of signals
     for (auto& [pair, pairSignals] : signalsByPair) {
         // Check for conflicting directions
         bool hasConflicts = false;
         Direction firstDirection = pairSignals[0].direction;
         
         for (size_t i = 1; i < pairSignals.size(); ++i) {
             if (pairSignals[i].direction != firstDirection) {
                 hasConflicts = true;
                 break;
             }
         }
         
         if (!hasConflicts) {
             // No conflicts, keep all signals
             resolvedSignals.insert(resolvedSignals.end(), pairSignals.begin(), pairSignals.end());
             continue;
         }
         
         // Handle conflicts based on strategy
         switch (signal_params_.conflict_resolution_strategy) {
             case ConflictResolutionStrategy::STRONGEST_ONLY: {
                 // Find the strongest signal
                 auto strongestIt = std::max_element(pairSignals.begin(), pairSignals.end(),
                     [](const TradingSignal& a, const TradingSignal& b) {
                         return a.strength < b.strength;
                     });
                 resolvedSignals.push_back(*strongestIt);
                 break;
             }
             
             case ConflictResolutionStrategy::HIGHEST_CONFIDENCE: {
                 // Find the signal with highest confidence
                 auto highestConfIt = std::max_element(pairSignals.begin(), pairSignals.end(),
                     [](const TradingSignal& a, const TradingSignal& b) {
                         return a.confidence < b.confidence;
                     });
                 resolvedSignals.push_back(*highestConfIt);
                 break;
             }
             
             case ConflictResolutionStrategy::STRENGTH_WEIGHTED: {
                 // Calculate net signal by weighting all signals
                 double buyStrength = 0.0;
                 double sellStrength = 0.0;
                 
                 for (const auto& signal : pairSignals) {
                     double weight = signal.strength * signal.confidence;
                     if (signal.direction == Direction::BUY) {
                         buyStrength += weight;
                     } else {
                         sellStrength += weight;
                     }
                 }
                 
                 // Only generate a signal if there's a clear winner
                 if (std::abs(buyStrength - sellStrength) > 0.2) {
                     // Create a new composite signal
                     TradingSignal compositeSignal = pairSignals[0]; // Start with first signal as template
                     
                     // Set new ID
                     std::stringstream ss;
                     ss << "COMP-" << std::chrono::system_clock::now().time_since_epoch().count();
                     compositeSignal.id = ss.str();
                     
                     // Update direction, strength, and confidence
                     if (buyStrength > sellStrength) {
                         compositeSignal.direction = Direction::BUY;
                         compositeSignal.strength = buyStrength / (buyStrength + sellStrength);
                     } else {
                         compositeSignal.direction = Direction::SELL;
                         compositeSignal.strength = sellStrength / (buyStrength + sellStrength);
                     }
                     
                     // Average confidence is reduced due to conflicting signals
                     compositeSignal.confidence = (buyStrength + sellStrength) / pairSignals.size() * 0.8;
                     
                     // Add to resolved signals
                     resolvedSignals.push_back(compositeSignal);
                 }
                 break;
             }
             
             case ConflictResolutionStrategy::CANCEL_ALL:
                 // In case of conflict, generate no signals
                 break;
         }
     }
     
     return resolvedSignals;
 }
 
 } // namespace strategy
 } // namespace trading