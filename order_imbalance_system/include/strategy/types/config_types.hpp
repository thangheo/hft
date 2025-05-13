/**
 * @file config_types.hpp
 * @brief Configuration structures for strategy components
 * 
 * This file defines configuration structures needed for various strategy components.
 * 
 * @author Trading System Team
 * @date May 13, 2025
 */

 #pragma once

 #include "types.hpp"
 #include <string>
 
 namespace trading {
 namespace strategy {
 
 /**
  * Configuration for OrderBookAnalyzer
  */
 struct OrderBookAnalyzerConfig {
     int depth = 10;                        // Default depth to analyze
     int historySize = 1000;                // Size of history to maintain
     double priceWeightFactor = 0.5;        // Weight factor for price-weighted metrics
     
     OrderBookAnalyzerConfig() = default;
 };
 
/**
 * Configuration for ImbalanceDetection
 */
struct ImbalanceDetectionConfig {
    // Strategy identification
    std::string strategy_id;                     // Unique identifier for the strategy
    std::string version = "1.0";                 // Version of the strategy
    
    // Core thresholds with defaults
    double volumeImbalanceThreshold = 0.7;       // Threshold for volume imbalance detection
    double priceLevelImbalanceThreshold = 0.6;   // Threshold for price level imbalance
    double liquidityRemovalThreshold = 0.5;      // Threshold for liquidity removal detection
    
    // Window and history settings
    int detectionWindowMs = 1000;                // Detection window in milliseconds
    int maxHistorySize = 1000;                   // Maximum history size for metrics
    
    // Advanced configuration
    double global_sensitivity = 1.0;             // Global sensitivity for all thresholds
    
    // Dynamic configuration maps
    std::map<std::string, double> thresholds;    // Thresholds for different imbalance types   
    std::map<std::string, double> parameters;    // General parameters for the strategy
    std::map<std::string, std::string> string_parameters; // String parameters
    
    ImbalanceDetectionConfig() = default;
    
    // Helper method to get threshold with global sensitivity applied
    double getAdjustedThreshold(const std::string& name, double default_value) const {
        auto it = thresholds.find(name);
        if (it != thresholds.end()) {
            return it->second * global_sensitivity;
        }
        return default_value * global_sensitivity;
    }
    
    // Helper to get parameter
    double getParameter(const std::string& name, double default_value) const {
        auto it = parameters.find(name);
        if (it != parameters.end()) {
            return it->second;
        }
        return default_value;
    }
    
    // Helper to get string parameter
    std::string getStringParameter(const std::string& name, const std::string& default_value) const {
        auto it = string_parameters.find(name);
        if (it != string_parameters.end()) {
            return it->second;
        }
        return default_value;
    }
};
 /**
  * Configuration for LiquidityFlowMonitor
  */
 struct LiquidityFlowConfig {
     int historySize = 1000;                // Size of history to maintain
     double flowImbalanceThreshold = 0.6;   // Threshold for flow imbalance detection
     
     LiquidityFlowConfig() = default;
 };
 
 /**
  * Configuration for VolumeProfileAnalyzer
  */
 struct VolumeProfileConfig {
     int timeWindowMs = 60000;              // Time window for volume profile in milliseconds
     int priceLevels = 20;                  // Number of price levels to track
     
     VolumeProfileConfig() = default;
 };
 
 /**
  * Configuration for ImbalanceSignalGenerator
  */
 struct SignalGeneratorConfig {
     double minSignalStrength = 0.1;        // Minimum signal strength to generate
     double confidenceThreshold = 0.3;      // Minimum confidence for signal generation
     uint64_t signalTtlMs = 5000;           // Time-to-live for signals in milliseconds
     
     SignalGeneratorConfig() = default;
 };
 
 /**
  * Configuration for MomentumCorrelation
  */
 struct MomentumCorrelationConfig {
     int correlationWindowMs = 10000;       // Window for correlation analysis in milliseconds
     int predictionHorizonMs = 5000;        // Prediction horizon in milliseconds
     
     MomentumCorrelationConfig() = default;
 };
 
 /**
  * Configuration for ThresholdManager
  */
 struct ThresholdManagerConfig {
     double globalSensitivity = 1.0;        // Global sensitivity factor
     double volatilityAdjustmentFactor = 0.5; // Factor for volatility adjustment
     
     ThresholdManagerConfig() = default;
 };
 
 /**
  * Configuration for SignalDecayModel
  */
 struct SignalDecayConfig {
     double defaultDecayRate = 0.1;         // Default decay rate per second
     double minSignalStrength = 0.05;       // Minimum signal strength before expiration
     
     SignalDecayConfig() = default;
 };
 
 /**
  * Configuration for ExchangeCalibration
  */
 struct ExchangeCalibrationConfig {
     bool enableAutoCalibration = true;     // Enable automatic calibration
     int calibrationIntervalMs = 3600000;   // Calibration interval in milliseconds (1 hour)
     
     ExchangeCalibrationConfig() = default;
 };
 
 /**
  * Configuration for AdaptiveMechanism
  */
 struct AdaptiveMechanismConfig {
     bool enableAdaptation = true;          // Enable adaptation
     double adaptationRate = 0.05;          // Rate of parameter adaptation
     int adaptationIntervalMs = 3600000;    // Adaptation interval in milliseconds (1 hour)
     
     AdaptiveMechanismConfig() = default;
 };
 
 /**
  * Extended StrategyConfig with component-specific configurations
  */
 struct ExtendedStrategyConfig : public StrategyConfig {
     OrderBookAnalyzerConfig orderBookAnalyzerConfig;
     ImbalanceDetectionConfig imbalanceDetectionConfig;
     LiquidityFlowConfig liquidityFlowConfig;
     VolumeProfileConfig volumeProfileConfig;
     SignalGeneratorConfig signalGeneratorConfig;
     MomentumCorrelationConfig momentumCorrelationConfig;
     ThresholdManagerConfig thresholdManagerConfig;
     SignalDecayConfig signalDecayConfig;
     ExchangeCalibrationConfig exchangeCalibrationConfig;
     AdaptiveMechanismConfig adaptiveMechanismConfig;
     
     // Map of trading pairs to monitor (symbol -> exchange ID)
     std::map<std::string, core::ExchangeId> tradingPairs;
     
     // Default parameters for strategy
     ParameterSet defaultParameters;
     
     ExtendedStrategyConfig() = default;
     
     // Constructor that takes base StrategyConfig
     ExtendedStrategyConfig(const StrategyConfig& baseConfig) 
         : StrategyConfig(baseConfig) {}
 };
 
  //AnalyzerConfig 
    // struct AnalyzerConfig {
    //     std::string analyzer_id;                // Unique identifier for the analyzer
    //     std::string version = "1.0";            // Version of the analyzer
    //     int depth = 10;                         // Depth to analyze
    //     int historySize = 1000;                 // Size of history to maintain
    //     double priceWeightFactor = 0.5;         // Weight factor for price-weighted metrics
        
    //     AnalyzerConfig() = default;
    // };
 } // namespace strategy
 } // namespace trading