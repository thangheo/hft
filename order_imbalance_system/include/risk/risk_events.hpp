/**
 * Event definitions for risk-related notifications
 *
 * File: risk_events.hpp
 * Created: 2025-04-29
 */

 #pragma once

 #include "risk_types.hpp"
 #include <functional>
 #include <string>
 #include <variant>
 
 namespace risk {
 
 /**
  * Enum for different types of risk events
  */
 enum class RiskEventType {
     LIMIT_BREACH,
     CIRCUIT_BREAKER_TRIGGERED,
     STOP_LOSS_TRIGGERED,
     DRAWDOWN_ALERT,
     POSITION_RECONCILIATION_MISMATCH,
     BALANCE_RECONCILIATION_MISMATCH,
     RISK_MODE_CHANGE,
     PARAMETER_UPDATE,
     EMERGENCY_ACTION
 };
 
 /**
  * Base class for all risk events
  */
 struct RiskEvent {
     RiskEventType type;
     Timestamp timestamp;
     std::string description;
     
     RiskEvent(RiskEventType eventType, Timestamp eventTime, std::string desc)
         : type(eventType), timestamp(eventTime), description(std::move(desc)) {}
         
     virtual ~RiskEvent() = default;
 };
 
 /**
  * Event for when a risk limit is breached
  */
 struct LimitBreachEvent : public RiskEvent {
     ExchangeId exchangeId;
     TradingPair tradingPair;
     StrategyId strategyId;
     ViolationType violationType;
     double limitValue;
     double actualValue;
     
     LimitBreachEvent(
         Timestamp eventTime,
         ExchangeId exchange,
         TradingPair pair,
         StrategyId strategy,
         ViolationType type,
         double limit,
         double actual,
         std::string desc
     ) : RiskEvent(RiskEventType::LIMIT_BREACH, eventTime, std::move(desc)),
         exchangeId(std::move(exchange)),
         tradingPair(std::move(pair)),
         strategyId(std::move(strategy)),
         violationType(type),
         limitValue(limit),
         actualValue(actual) {}
 };
 
 /**
  * Event for when a circuit breaker is triggered
  */
 struct CircuitBreakerEvent : public RiskEvent {
     CircuitBreakerState oldState;
     CircuitBreakerState newState;
     std::string metricName;
     double thresholdValue;
     double actualValue;
     std::optional<Timestamp> resumeTime;
     
     CircuitBreakerEvent(
         Timestamp eventTime,
         CircuitBreakerState oldSt,
         CircuitBreakerState newSt,
         std::string metric,
         double threshold,
         double actual,
         std::optional<Timestamp> resume,
         std::string desc
     ) : RiskEvent(RiskEventType::CIRCUIT_BREAKER_TRIGGERED, eventTime, std::move(desc)),
         oldState(oldSt),
         newState(newSt),
         metricName(std::move(metric)),
         thresholdValue(threshold),
         actualValue(actual),
         resumeTime(resume) {}
 };
 
 /**
  * Event for when a stop loss is triggered
  */
 struct StopLossEvent : public RiskEvent {
     ExchangeId exchangeId;
     TradingPair tradingPair;
     StrategyId strategyId;
     double positionSize;
     double entryPrice;
     double currentPrice;
     double lossPercentage;
     bool liquidationInitiated;
     
     StopLossEvent(
         Timestamp eventTime,
         ExchangeId exchange,
         TradingPair pair,
         StrategyId strategy,
         double size,
         double entry,
         double current,
         double lossPercent,
         bool liquidation,
         std::string desc
     ) : RiskEvent(RiskEventType::STOP_LOSS_TRIGGERED, eventTime, std::move(desc)),
         exchangeId(std::move(exchange)),
         tradingPair(std::move(pair)),
         strategyId(std::move(strategy)),
         positionSize(size),
         entryPrice(entry),
         currentPrice(current),
         lossPercentage(lossPercent),
         liquidationInitiated(liquidation) {}
 };
 
 /**
  * Event for drawdown alerts
  */
 struct DrawdownEvent : public RiskEvent {
     double drawdownPercentage;
     double highWatermark;
     double currentValue;
     StrategyId strategyId;
     Timestamp highWatermarkTime;
     bool thresholdBreached;
     
     DrawdownEvent(
         Timestamp eventTime,
         double drawdown,
         double hwm,
         double current,
         StrategyId strategy,
         Timestamp hwmTime,
         bool breached,
         std::string desc
     ) : RiskEvent(RiskEventType::DRAWDOWN_ALERT, eventTime, std::move(desc)),
         drawdownPercentage(drawdown),
         highWatermark(hwm),
         currentValue(current),
         strategyId(std::move(strategy)),
         highWatermarkTime(hwmTime),
         thresholdBreached(breached) {}
 };
 
 /**
  * Event for position reconciliation mismatches
  */
 struct PositionReconciliationEvent : public RiskEvent {
     ExchangeId exchangeId;
     TradingPair tradingPair;
     double expectedQuantity;
     double actualQuantity;
     double discrepancyPercentage;
     bool autoCorrect;
     
     PositionReconciliationEvent(
         Timestamp eventTime,
         ExchangeId exchange,
         TradingPair pair,
         double expected,
         double actual,
         double discrepancy,
         bool correction,
         std::string desc
     ) : RiskEvent(RiskEventType::POSITION_RECONCILIATION_MISMATCH, eventTime, std::move(desc)),
         exchangeId(std::move(exchange)),
         tradingPair(std::move(pair)),
         expectedQuantity(expected),
         actualQuantity(actual),
         discrepancyPercentage(discrepancy),
         autoCorrect(correction) {}
 };
 
 /**
  * Event for risk mode changes
  */
 struct RiskModeEvent : public RiskEvent {
     RiskManagerMode oldMode;
     RiskManagerMode newMode;
     std::string changeReason;
     
     RiskModeEvent(
         Timestamp eventTime,
         RiskManagerMode oldMd,
         RiskManagerMode newMd,
         std::string reason,
         std::string desc
     ) : RiskEvent(RiskEventType::RISK_MODE_CHANGE, eventTime, std::move(desc)),
         oldMode(oldMd),
         newMode(newMd),
         changeReason(std::move(reason)) {}
 };
 
 /**
  * Type alias for risk event variant
  */
 using RiskEventVariant = std::variant<
     LimitBreachEvent,
     CircuitBreakerEvent,
     StopLossEvent,
     DrawdownEvent,
     PositionReconciliationEvent,
     RiskModeEvent
 >;
 
 /**
  * Type alias for risk event callback function
  */
 using RiskEventCallback = std::function<void(const RiskEvent&)>;
 
 } // namespace risk

