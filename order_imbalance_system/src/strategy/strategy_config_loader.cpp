/**
 * @file strategy_config_loader.cpp
 * @brief Utility functions for loading strategy configuration from files
 * 
 * Created on: May 5, 2025
 */

 #include "strategy/engine/strategy_engine.hpp"
 #include <nlohmann/json.hpp>
 #include <fstream>
 #include <stdexcept>
 #include <filesystem>
 #include "common/logger.hpp"
 
 namespace trading {
 namespace strategy {
 
 using json = nlohmann::json;
 
 /**
  * @brief Load strategy configuration from a JSON file
  * 
  * @param filePath Path to the JSON configuration file
  * @return StrategyConfig Configuration structure
  * @throws std::runtime_error if file cannot be read or has invalid format
  */
 StrategyConfig loadStrategyConfigFromFile(const std::string& filePath) {
     TRADING_LOG_INFO("Loading strategy configuration from file: {}", filePath);
     
     // Check if file exists
     if (!std::filesystem::exists(filePath)) {
         TRADING_LOG_ERROR("Configuration file not found: {}", filePath);
         throw std::runtime_error("Configuration file not found: " + filePath);
     }
     
     // Open the file
     std::ifstream file(filePath);
     if (!file.is_open()) {
         TRADING_LOG_ERROR("Failed to open configuration file: {}", filePath);
         throw std::runtime_error("Failed to open configuration file: " + filePath);
     }
     
     try {
         // Parse JSON
         json configJson;
         file >> configJson;
         
         // Create config object
         StrategyConfig config;
         
         // Parse general settings
         config.strategyId = configJson.value("strategyId", "default_strategy");
         config.version = configJson.value("version", "1.0.0");
         
         // Parse trading pairs
         if (configJson.contains("tradingPairs") && configJson["tradingPairs"].is_array()) {
             for (const auto& pair : configJson["tradingPairs"]) {
                 std::string symbol = pair["symbol"];
                 ExchangeId exchangeId = static_cast<ExchangeId>(pair["exchangeId"].get<int>());
                 config.tradingPairs[symbol] = exchangeId;
             }
         }
         
         // Parse default parameters
         if (configJson.contains("parameters") && configJson["parameters"].is_object()) {
             const auto& params = configJson["parameters"];
             
             // Parse numeric parameters
             if (params.contains("numeric") && params["numeric"].is_object()) {
                 for (auto it = params["numeric"].begin(); it != params["numeric"].end(); ++it) {
                     config.defaultParameters.numericParams[it.key()] = it.value().get<double>();
                 }
             }
             
             // Parse string parameters
             if (params.contains("string") && params["string"].is_object()) {
                 for (auto it = params["string"].begin(); it != params["string"].end(); ++it) {
                     config.defaultParameters.stringParams[it.key()] = it.value().get<std::string>();
                 }
             }
             
             // Parse boolean parameters
             if (params.contains("boolean") && params["boolean"].is_object()) {
                 for (auto it = params["boolean"].begin(); it != params["boolean"].end(); ++it) {
                     config.defaultParameters.boolParams[it.key()] = it.value().get<bool>();
                 }
             }
             
             // Parse array parameters
             if (params.contains("array") && params["array"].is_object()) {
                 for (auto it = params["array"].begin(); it != params["array"].end(); ++it) {
                     if (it.value().is_array()) {
                         std::vector<double> values;
                         for (const auto& val : it.value()) {
                             values.push_back(val.get<double>());
                         }
                         config.defaultParameters.arrayParams[it.key()] = values;
                     }
                 }
             }
         }
         
         // Parse component-specific configs
         if (configJson.contains("components") && configJson["components"].is_object()) {
             const auto& components = configJson["components"];
             
             // Parse OrderBookAnalyzer config
             if (components.contains("orderBookAnalyzer")) {
                 config.orderBookAnalyzerConfig = parseComponentConfig(components["orderBookAnalyzer"]);
             }
             
             // Parse ImbalanceDetectionEngine config
             if (components.contains("imbalanceDetector")) {
                 config.imbalanceDetectionConfig = parseComponentConfig(components["imbalanceDetector"]);
             }
             
             // Parse LiquidityFlowMonitor config
             if (components.contains("liquidityMonitor")) {
                 config.liquidityFlowConfig = parseComponentConfig(components["liquidityMonitor"]);
             }
             
             // Parse VolumeProfileAnalyzer config
             if (components.contains("volumeProfileAnalyzer")) {
                 config.volumeProfileConfig = parseComponentConfig(components["volumeProfileAnalyzer"]);
             }
             
             // Parse ImbalanceSignalGenerator config
             if (components.contains("signalGenerator")) {
                 config.signalGeneratorConfig = parseComponentConfig(components["signalGenerator"]);
             }
             
             // Parse MomentumCorrelation config
             if (components.contains("momentumCorrelator")) {
                 config.momentumCorrelationConfig = parseComponentConfig(components["momentumCorrelator"]);
             }
             
             // Parse ThresholdManager config
             if (components.contains("thresholdManager")) {
                 config.thresholdManagerConfig = parseComponentConfig(components["thresholdManager"]);
             }
             
             // Parse SignalDecayModel config
             if (components.contains("signalDecayModel")) {
                 config.signalDecayConfig = parseComponentConfig(components["signalDecayModel"]);
             }
             
             // Parse ExchangeCalibration config
             if (components.contains("exchangeCalibration")) {
                 config.exchangeCalibrationConfig = parseComponentConfig(components["exchangeCalibration"]);
             }
             
             // Parse AdaptiveMechanism config
             if (components.contains("adaptiveMechanism")) {
                 config.adaptiveMechanismConfig = parseComponentConfig(components["adaptiveMechanism"]);
             }
         }
         
         TRADING_LOG_INFO("Successfully loaded strategy configuration: id={}, version={}, tradingPairs={}",
                        config.strategyId, config.version, config.tradingPairs.size());
         
         return config;
     } catch (const std::exception& e) {
         TRADING_LOG_ERROR("Error parsing strategy configuration file: {}", e.what());
         throw std::runtime_error("Error parsing strategy configuration file: " + std::string(e.what()));
     }
 }
 
 /**
  * @brief Parse component-specific configuration from JSON object
  * 
  * @param componentJson JSON object containing component configuration
  * @return ComponentConfig Component-specific configuration
  */
 ComponentConfig parseComponentConfig(const json& componentJson) {
     ComponentConfig config;
     
     // Parse enabled flag
     config.enabled = componentJson.value("enabled", true);
     
     // Parse numeric parameters
     if (componentJson.contains("parameters") && componentJson["parameters"].is_object()) {
         const auto& params = componentJson["parameters"];
         
         for (auto it = params.begin(); it != params.end(); ++it) {
             if (it.value().is_number()) {
                 config.parameters[it.key()] = it.value().get<double>();
             } else if (it.value().is_string()) {
                 config.stringParameters[it.key()] = it.value().get<std::string>();
             } else if (it.value().is_boolean()) {
                 config.boolParameters[it.key()] = it.value().get<bool>();
             }
         }
     }
     
     return config;
 }
 
 /**
  * @brief Save strategy configuration to a JSON file
  * 
  * @param config Configuration to save
  * @param filePath Path to the output JSON file
  * @return bool True if successful, false otherwise
  */
 bool saveStrategyConfigToFile(const StrategyConfig& config, const std::string& filePath) {
     try {
         // Create JSON object
         json configJson;
         
         // General settings
         configJson["strategyId"] = config.strategyId;
         configJson["version"] = config.version;
         
         // Trading pairs
         json pairsJson = json::array();
         for (const auto& [symbol, exchangeId] : config.tradingPairs) {
             json pairJson;
             pairJson["symbol"] = symbol;
             pairJson["exchangeId"] = static_cast<int>(exchangeId);
             pairsJson.push_back(pairJson);
         }
         configJson["tradingPairs"] = pairsJson;
         
         // Parameters
         json paramsJson;
         
         // Numeric parameters
         json numericJson;
         for (const auto& [key, value] : config.defaultParameters.numericParams) {
             numericJson[key] = value;
         }
         paramsJson["numeric"] = numericJson;
         
         // String parameters
         json stringJson;
         for (const auto& [key, value] : config.defaultParameters.stringParams) {
             stringJson[key] = value;
         }
         paramsJson["string"] = stringJson;
         
         // Boolean parameters
         json boolJson;
         for (const auto& [key, value] : config.defaultParameters.boolParams) {
             boolJson[key] = value;
         }
         paramsJson["boolean"] = boolJson;
         
         // Array parameters
         json arrayJson;
         for (const auto& [key, value] : config.defaultParameters.arrayParams) {
             arrayJson[key] = value;
         }
         paramsJson["array"] = arrayJson;
         
         configJson["parameters"] = paramsJson;
         
         // Component configurations
         json componentsJson;
         
         // OrderBookAnalyzer
         componentsJson["orderBookAnalyzer"] = componentConfigToJson(config.orderBookAnalyzerConfig);
         
         // ImbalanceDetectionEngine
         componentsJson["imbalanceDetector"] = componentConfigToJson(config.imbalanceDetectionConfig);
         
         // LiquidityFlowMonitor
         componentsJson["liquidityMonitor"] = componentConfigToJson(config.liquidityFlowConfig);
         
         // VolumeProfileAnalyzer
         componentsJson["volumeProfileAnalyzer"] = componentConfigToJson(config.volumeProfileConfig);
         
         // ImbalanceSignalGenerator
         componentsJson["signalGenerator"] = componentConfigToJson(config.signalGeneratorConfig);
         
         // MomentumCorrelation
         componentsJson["momentumCorrelator"] = componentConfigToJson(config.momentumCorrelationConfig);
         
         // ThresholdManager
         componentsJson["thresholdManager"] = componentConfigToJson(config.thresholdManagerConfig);
         
         // SignalDecayModel
         componentsJson["signalDecayModel"] = componentConfigToJson(config.signalDecayConfig);
         
         // ExchangeCalibration
         componentsJson["exchangeCalibration"] = componentConfigToJson(config.exchangeCalibrationConfig);
         
         // AdaptiveMechanism
         componentsJson["adaptiveMechanism"] = componentConfigToJson(config.adaptiveMechanismConfig);
         
         configJson["components"] = componentsJson;
         
         // Write to file
         std::ofstream file(filePath);
         if (!file.is_open()) {
             TRADING_LOG_ERROR("Failed to open file for writing: {}", filePath);
             return false;
         }
         
         file << configJson.dump(4);  // Pretty print with 4 spaces
         file.close();
         
         TRADING_LOG_INFO("Successfully saved strategy configuration to file: {}", filePath);
         return true;
     } catch (const std::exception& e) {
         TRADING_LOG_ERROR("Error saving strategy configuration: {}", e.what());
         return false;
     }
 }
 
 /**
  * @brief Convert component config to JSON object
  * 
  * @param config Component configuration
  * @return json JSON representation of the component configuration
  */
 json componentConfigToJson(const ComponentConfig& config) {
     json componentJson;
     
     // Enabled flag
     componentJson["enabled"] = config.enabled;
     
     // Parameters
     json paramsJson;
     
     // Numeric parameters
     for (const auto& [key, value] : config.parameters) {
         paramsJson[key] = value;
     }
     
     // String parameters
     for (const auto& [key, value] : config.stringParameters) {
         paramsJson[key] = value;
     }
     
     // Boolean parameters
     for (const auto& [key, value] : config.boolParameters) {
         paramsJson[key] = value;
     }
     
     componentJson["parameters"] = paramsJson;
     
     return componentJson;
 }
 
 /**
  * @brief Load exchange-specific parameter sets
  * 
  * @param filePath Path to the JSON file containing exchange-specific parameters
  * @return std::map<ExchangeId, ParameterSet> Map of exchange ID to parameter set
  */
 std::map<ExchangeId, ParameterSet> loadExchangeSpecificParameters(const std::string& filePath) {
     std::map<ExchangeId, ParameterSet> result;
     
     try {
         // Check if file exists
         if (!std::filesystem::exists(filePath)) {
             TRADING_LOG_WARN("Exchange-specific parameters file not found: {}", filePath);
             return result;
         }
         
         // Open the file
         std::ifstream file(filePath);
         if (!file.is_open()) {
             TRADING_LOG_ERROR("Failed to open exchange-specific parameters file: {}", filePath);
             return result;
         }
         
         // Parse JSON
         json paramsJson;
         file >> paramsJson;
         
         // Iterate through exchanges
         for (auto it = paramsJson.begin(); it != paramsJson.end(); ++it) {
             // Parse exchange ID
             ExchangeId exchangeId = static_cast<ExchangeId>(std::stoi(it.key()));
             
             // Parse parameter set
             ParameterSet params;
             
             const auto& exchangeParams = it.value();
             
             // Parse numeric parameters
             if (exchangeParams.contains("numeric") && exchangeParams["numeric"].is_object()) {
                 for (auto paramIt = exchangeParams["numeric"].begin(); paramIt != exchangeParams["numeric"].end(); ++paramIt) {
                     params.numericParams[paramIt.key()] = paramIt.value().get<double>();
                 }
             }
             
             // Parse string parameters
             if (exchangeParams.contains("string") && exchangeParams["string"].is_object()) {
                 for (auto paramIt = exchangeParams["string"].begin(); paramIt != exchangeParams["string"].end(); ++paramIt) {
                     params.stringParams[paramIt.key()] = paramIt.value().get<std::string>();
                 }
             }
             
             // Parse boolean parameters
             if (exchangeParams.contains("boolean") && exchangeParams["boolean"].is_object()) {
                 for (auto paramIt = exchangeParams["boolean"].begin(); paramIt != exchangeParams["boolean"].end(); ++paramIt) {
                     params.boolParams[paramIt.key()] = paramIt.value().get<bool>();
                 }
             }
             
             // Parse array parameters
             if (exchangeParams.contains("array") && exchangeParams["array"].is_object()) {
                 for (auto paramIt = exchangeParams["array"].begin(); paramIt != exchangeParams["array"].end(); ++paramIt) {
                     if (paramIt.value().is_array()) {
                         std::vector<double> values;
                         for (const auto& val : paramIt.value()) {
                             values.push_back(val.get<double>());
                         }
                         params.arrayParams[paramIt.key()] = values;
                     }
                 }
             }
             
             // Add to result map
             result[exchangeId] = params;
         }
         
         TRADING_LOG_INFO("Loaded exchange-specific parameters for {} exchanges", result.size());
         return result;
     } catch (const std::exception& e) {
         TRADING_LOG_ERROR("Error loading exchange-specific parameters: {}", e.what());
         return result;
     }
 }
 
 } // namespace strategy
 } // namespace trading