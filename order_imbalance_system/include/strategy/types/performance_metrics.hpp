/**
 * @file strategy/types/performance_metrics.hpp
 * @brief Performance metrics for strategy evaluation
 * 
 * This file defines performance metrics used to evaluate trading strategies.
 * 
 * @author Trading System Team
 * @date May 12, 2025
 */

 #pragma once

 namespace trading {
 namespace strategy {
 
 /**
  * Performance metrics for strategy evaluation
  */
 struct PerformanceMetrics {
     double pnl;             // Profit and loss
     double sharpe_ratio;    // Sharpe ratio
     double max_drawdown;    // Maximum drawdown
     double win_rate;        // Win rate (percentage)
     double profit_factor;   // Profit factor (gross profit / gross loss)
     int trade_count;        // Number of trades
     double avg_trade_pnl;   // Average P&L per trade
     
     // Default constructor with zeroed metrics
     PerformanceMetrics()
         : pnl(0.0), sharpe_ratio(0.0), max_drawdown(0.0),
           win_rate(0.0), profit_factor(0.0), trade_count(0), avg_trade_pnl(0.0) {}
 };
 
 } // namespace strategy
 } // namespace trading