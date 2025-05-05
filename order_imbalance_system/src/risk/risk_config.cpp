/**
 * Implementation of Risk configuration structures and parameter definitions
 *
 * File: risk_config.cpp
 * Created: 2025-05-05
 */

 #include "risk_config.hpp"
 #include <nlohmann/json.hpp>
 #include <fstream>
 #include <iostream>
 #include <stdexcept>
 
 using json = nlohmann::json;
 namespace trading{
 namespace risk {
 
 // Default constructor with reasonable default values
 RiskConfig::RiskConfig() {
     // Initialize position limit config
     m_positionLimitConfig.defaultLimit = 1000.0;
     m_positionLimitConfig.enforceStrictly = true;
 
     // Initialize exposure limit config
     m_exposureLimitConfig.maxTotalExposure = 10000.0;
     m_exposureLimitConfig.maxAssetExposure = 2000.0;
     m_exposureLimitConfig.maxStrategyExposure = 5000.0;
     m_exposureLimitConfig.maxExchangeExposure = 8000.0;
     m_exposureLimitConfig.volatilityMultiplier = 1.5;
     m_exposureLimitConfig.includeUnrealizedPnl = true;
 
     // Initialize circuit breaker config
     m_circuitBreakerConfig.warningThresholds["drawdown"] = 0.05;
     m_circuitBreakerConfig.criticalThresholds["drawdown"] = 0.10;
     m_circuitBreakerConfig.coolingPeriod = std::chrono::seconds(300); // 5 minutes
     m_circuitBreakerConfig.warmupPeriod = std::chrono::seconds(60);   // 1 minute
     m_circuitBreakerConfig.autoResume = true;
     m_circuitBreakerConfig.maxTriggersPerDay = 3;
 
     // Initialize stop loss config
     m_stopLossConfig.maxLossPercentage = 0.05;
     m_stopLossConfig.trailingStopDistance = 0.02;
     m_stopLossConfig.enableTrailingStop = true;
     m_stopLossConfig.forceLiquidation = true;
     m_stopLossConfig.liquidationDelay = std::chrono::seconds(5);
 
     // Initialize drawdown config
     m_drawdownConfig.warningThreshold = 0.10;
     m_drawdownConfig.criticalThreshold = 0.20;
     m_drawdownConfig.lookbackPeriod = std::chrono::hours(24);
     m_drawdownConfig.enableAutoRiskReduction = true;
     m_drawdownConfig.riskReductionFactor = 0.5;
 
     // Initialize fat finger config
     m_fatFingerConfig.maxPriceDeviation = 0.05;
     m_fatFingerConfig.maxOrderSize = 10.0;
     m_fatFingerConfig.typicalOrderSize = 1.0;
     m_fatFingerConfig.notionalValueLimit = 10000.0;
     m_fatFingerConfig.enforceAdditionalVerification = true;
 
     // Initialize order validation config
     m_orderValidationConfig.validateMinOrderSize = true;
     m_orderValidationConfig.validatePriceRange = true;
     m_orderValidationConfig.maxPriceDeviationPercent = 0.1;
     m_orderValidationConfig.enforceSymbolWhitelist = false;
 
     // Initialize balance reconciliation config
     m_balanceReconciliationConfig.reconciliationFrequency = std::chrono::minutes(15);
     m_balanceReconciliationConfig.maxDiscrepancyPercent = 0.01;
     m_balanceReconciliationConfig.autoCorrectDiscrepancies = false;
     m_balanceReconciliationConfig.alertOnDiscrepancy = true;
     m_balanceReconciliationConfig.maxConsecutiveFailures = 3;
 
     // Initialize rebalancing config
     m_rebalancingConfig.rebalanceThreshold = 0.1;
     m_rebalancingConfig.checkFrequency = std::chrono::hours(1);
     m_rebalancingConfig.autoRebalance = true;
     m_rebalancingConfig.preferLowFeeExchanges = true;
     m_rebalancingConfig.minimumTimeBetweenRebalances = std::chrono::seconds(3600);
 
     // Initialize collateral config
     m_collateralConfig.minFreeCollateralRatio = 0.1;
     m_collateralConfig.targetFreeCollateralRatio = 0.2;
     m_collateralConfig.enableEmergencyFunding = false;
     m_collateralConfig.maxFundingAmount = 10000.0;
 
     // Initialize logging config
     m_loggingConfig.logLevel = "INFO";
     m_loggingConfig.logDirectory = "/var/log/riskmanagement";
     m_loggingConfig.enableDetailedLogs = true;
     m_loggingConfig.logToConsole = true;
     m_loggingConfig.logToFile = true;
     m_loggingConfig.logRetentionPeriod = std::chrono::days(30);
 }
 
 // Constructor with file path
 RiskConfig::RiskConfig(const std::string& configFilePath) : RiskConfig() {
     if (!loadFromFile(configFilePath)) {
         throw ConfigurationException("Failed to load configuration from file: " + configFilePath);
     }
 }
 
 // Load configuration from file
 bool RiskConfig::loadFromFile(const std::string& configFilePath) {
     try {
         // Open the file
         std::ifstream file(configFilePath);
         if (!file.is_open()) {
             std::cerr << "Failed to open configuration file: " << configFilePath << std::endl;
             return false;
         }
         
         // Read the file into a string
         std::string jsonStr((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
         file.close();
         
         // Parse the JSON string
         return loadJsonConfiguration(jsonStr);
     } catch (const std::exception& e) {
         std::cerr << "Exception while loading configuration: " << e.what() << std::endl;
         return false;
     }
 }
 
 // Parse JSON configuration
 bool RiskConfig::loadJsonConfiguration(const std::string& jsonStr) {
     try {
         // Parse the JSON string
         json j = json::parse(jsonStr);
         
         // Load position limit config
         if (j.contains("positionLimit")) {
             auto& pl = j["positionLimit"];
             if (pl.contains("assetLimits") && pl["assetLimits"].is_object()) {
                 for (auto& [asset, limit] : pl["assetLimits"].items()) {
                     m_positionLimitConfig.assetLimits[asset] = limit.get<double>();
                 }
             }
             if (pl.contains("strategyLimits") && pl["strategyLimits"].is_object()) {
                 for (auto& [strategy, limit] : pl["strategyLimits"].items()) {
                     m_positionLimitConfig.strategyLimits[strategy] = limit.get<double>();
                 }
             }
             if (pl.contains("exchangeLimits") && pl["exchangeLimits"].is_object()) {
                 for (auto& [exchange, limit] : pl["exchangeLimits"].items()) {
                     m_positionLimitConfig.exchangeLimits[exchange] = limit.get<double>();
                 }
             }
             if (pl.contains("defaultLimit")) {
                 m_positionLimitConfig.defaultLimit = pl["defaultLimit"].get<double>();
             }
             if (pl.contains("enforceStrictly")) {
                 m_positionLimitConfig.enforceStrictly = pl["enforceStrictly"].get<bool>();
             }
         }
         
         // Load exposure limit config
         if (j.contains("exposureLimit")) {
             auto& el = j["exposureLimit"];
             if (el.contains("maxTotalExposure")) {
                 m_exposureLimitConfig.maxTotalExposure = el["maxTotalExposure"].get<double>();
             }
             if (el.contains("maxAssetExposure")) {
                 m_exposureLimitConfig.maxAssetExposure = el["maxAssetExposure"].get<double>();
             }
             if (el.contains("maxStrategyExposure")) {
                 m_exposureLimitConfig.maxStrategyExposure = el["maxStrategyExposure"].get<double>();
             }
             if (el.contains("maxExchangeExposure")) {
                 m_exposureLimitConfig.maxExchangeExposure = el["maxExchangeExposure"].get<double>();
             }
             if (el.contains("volatilityMultiplier")) {
                 m_exposureLimitConfig.volatilityMultiplier = el["volatilityMultiplier"].get<double>();
             }
             if (el.contains("includeUnrealizedPnl")) {
                 m_exposureLimitConfig.includeUnrealizedPnl = el["includeUnrealizedPnl"].get<bool>();
             }
         }
         
         // Load circuit breaker config
         if (j.contains("circuitBreaker")) {
             auto& cb = j["circuitBreaker"];
             if (cb.contains("warningThresholds") && cb["warningThresholds"].is_object()) {
                 for (auto& [metric, threshold] : cb["warningThresholds"].items()) {
                     m_circuitBreakerConfig.warningThresholds[metric] = threshold.get<double>();
                 }
             }
             if (cb.contains("criticalThresholds") && cb["criticalThresholds"].is_object()) {
                 for (auto& [metric, threshold] : cb["criticalThresholds"].items()) {
                     m_circuitBreakerConfig.criticalThresholds[metric] = threshold.get<double>();
                 }
             }
             if (cb.contains("coolingPeriod")) {
                 m_circuitBreakerConfig.coolingPeriod = std::chrono::seconds(cb["coolingPeriod"].get<int>());
             }
             if (cb.contains("warmupPeriod")) {
                 m_circuitBreakerConfig.warmupPeriod = std::chrono::seconds(cb["warmupPeriod"].get<int>());
             }
             if (cb.contains("autoResume")) {
                 m_circuitBreakerConfig.autoResume = cb["autoResume"].get<bool>();
             }
             if (cb.contains("maxTriggersPerDay")) {
                 m_circuitBreakerConfig.maxTriggersPerDay = cb["maxTriggersPerDay"].get<int>();
             }
         }
         
         // Load stop loss config
         if (j.contains("stopLoss")) {
             auto& sl = j["stopLoss"];
             if (sl.contains("maxLossPercentage")) {
                 m_stopLossConfig.maxLossPercentage = sl["maxLossPercentage"].get<double>();
             }
             if (sl.contains("trailingStopDistance")) {
                 m_stopLossConfig.trailingStopDistance = sl["trailingStopDistance"].get<double>();
             }
             if (sl.contains("enableTrailingStop")) {
                 m_stopLossConfig.enableTrailingStop = sl["enableTrailingStop"].get<bool>();
             }
             if (sl.contains("forceLiquidation")) {
                 m_stopLossConfig.forceLiquidation = sl["forceLiquidation"].get<bool>();
             }
             if (sl.contains("liquidationDelay")) {
                 m_stopLossConfig.liquidationDelay = std::chrono::seconds(sl["liquidationDelay"].get<int>());
             }
         }
         
         // Load drawdown config
         if (j.contains("drawdown")) {
             auto& dd = j["drawdown"];
             if (dd.contains("warningThreshold")) {
                 m_drawdownConfig.warningThreshold = dd["warningThreshold"].get<double>();
             }
             if (dd.contains("criticalThreshold")) {
                 m_drawdownConfig.criticalThreshold = dd["criticalThreshold"].get<double>();
             }
             if (dd.contains("lookbackPeriodHours")) {
                 m_drawdownConfig.lookbackPeriod = std::chrono::hours(dd["lookbackPeriodHours"].get<int>());
             }
             if (dd.contains("enableAutoRiskReduction")) {
                 m_drawdownConfig.enableAutoRiskReduction = dd["enableAutoRiskReduction"].get<bool>();
             }
             if (dd.contains("riskReductionFactor")) {
                 m_drawdownConfig.riskReductionFactor = dd["riskReductionFactor"].get<double>();
             }
         }
         
         // Load fat finger config
         if (j.contains("fatFinger")) {
             auto& ff = j["fatFinger"];
             if (ff.contains("maxPriceDeviation")) {
                 m_fatFingerConfig.maxPriceDeviation = ff["maxPriceDeviation"].get<double>();
             }
             if (ff.contains("maxOrderSize")) {
                 m_fatFingerConfig.maxOrderSize = ff["maxOrderSize"].get<double>();
             }
             if (ff.contains("typicalOrderSize")) {
                 m_fatFingerConfig.typicalOrderSize = ff["typicalOrderSize"].get<double>();
             }
             if (ff.contains("notionalValueLimit")) {
                 m_fatFingerConfig.notionalValueLimit = ff["notionalValueLimit"].get<double>();
             }
             if (ff.contains("enforceAdditionalVerification")) {
                 m_fatFingerConfig.enforceAdditionalVerification = ff["enforceAdditionalVerification"].get<bool>();
             }
         }
         
         // Load order validation config
         if (j.contains("orderValidation")) {
             auto& ov = j["orderValidation"];
             if (ov.contains("validateMinOrderSize")) {
                 m_orderValidationConfig.validateMinOrderSize = ov["validateMinOrderSize"].get<bool>();
             }
             if (ov.contains("validatePriceRange")) {
                 m_orderValidationConfig.validatePriceRange = ov["validatePriceRange"].get<bool>();
             }
             if (ov.contains("minOrderSizes") && ov["minOrderSizes"].is_object()) {
                 for (auto& [exchangeId, pairs] : ov["minOrderSizes"].items()) {
                     ExchangeId exchange{exchangeId};
                     if (pairs.is_object()) {
                         for (auto& [symbolStr, minSize] : pairs.items()) {
                             TradingPair pair;
                             pair.symbol = symbolStr;
                             // Parse base/quote from symbol if needed
                             auto separatorPos = symbolStr.find("/");
                             if (separatorPos != std::string::npos) {
                                 pair.base = symbolStr.substr(0, separatorPos);
                                 pair.quote = symbolStr.substr(separatorPos + 1);
                             }
                             m_orderValidationConfig.minOrderSizes[exchange][pair] = minSize.get<double>();
                         }
                     }
                 }
             }
             if (ov.contains("maxPriceDeviationPercent")) {
                 m_orderValidationConfig.maxPriceDeviationPercent = ov["maxPriceDeviationPercent"].get<double>();
             }
             if (ov.contains("enforceSymbolWhitelist")) {
                 m_orderValidationConfig.enforceSymbolWhitelist = ov["enforceSymbolWhitelist"].get<bool>();
             }
             if (ov.contains("symbolWhitelist") && ov["symbolWhitelist"].is_array()) {
                 m_orderValidationConfig.symbolWhitelist.clear();
                 for (auto& symbol : ov["symbolWhitelist"]) {
                     m_orderValidationConfig.symbolWhitelist.push_back(symbol.get<std::string>());
                 }
             }
         }
         
         // Load balance reconciliation config
         if (j.contains("balanceReconciliation")) {
             auto& br = j["balanceReconciliation"];
             if (br.contains("reconciliationFrequencyMinutes")) {
                 m_balanceReconciliationConfig.reconciliationFrequency = 
                     std::chrono::minutes(br["reconciliationFrequencyMinutes"].get<int>());
             }
             if (br.contains("maxDiscrepancyPercent")) {
                 m_balanceReconciliationConfig.maxDiscrepancyPercent = br["maxDiscrepancyPercent"].get<double>();
             }
             if (br.contains("autoCorrectDiscrepancies")) {
                 m_balanceReconciliationConfig.autoCorrectDiscrepancies = br["autoCorrectDiscrepancies"].get<bool>();
             }
             if (br.contains("alertOnDiscrepancy")) {
                 m_balanceReconciliationConfig.alertOnDiscrepancy = br["alertOnDiscrepancy"].get<bool>();
             }
             if (br.contains("maxConsecutiveFailures")) {
                 m_balanceReconciliationConfig.maxConsecutiveFailures = br["maxConsecutiveFailures"].get<int>();
             }
         }
         
         // Load rebalancing config
         if (j.contains("rebalancing")) {
             auto& rb = j["rebalancing"];
             if (rb.contains("rebalanceThreshold")) {
                 m_rebalancingConfig.rebalanceThreshold = rb["rebalanceThreshold"].get<double>();
             }
             if (rb.contains("checkFrequencyHours")) {
                 m_rebalancingConfig.checkFrequency = std::chrono::hours(rb["checkFrequencyHours"].get<int>());
             }
             if (rb.contains("autoRebalance")) {
                 m_rebalancingConfig.autoRebalance = rb["autoRebalance"].get<bool>();
             }
             if (rb.contains("preferLowFeeExchanges")) {
                 m_rebalancingConfig.preferLowFeeExchanges = rb["preferLowFeeExchanges"].get<bool>();
             }
             if (rb.contains("minimumTimeBetweenRebalancesSeconds")) {
                 m_rebalancingConfig.minimumTimeBetweenRebalances = 
                     std::chrono::seconds(rb["minimumTimeBetweenRebalancesSeconds"].get<int>());
             }
         }
         
         // Load collateral config
         if (j.contains("collateral")) {
             auto& co = j["collateral"];
             if (co.contains("minFreeCollateralRatio")) {
                 m_collateralConfig.minFreeCollateralRatio = co["minFreeCollateralRatio"].get<double>();
             }
             if (co.contains("targetFreeCollateralRatio")) {
                 m_collateralConfig.targetFreeCollateralRatio = co["targetFreeCollateralRatio"].get<double>();
             }
             if (co.contains("enableEmergencyFunding")) {
                 m_collateralConfig.enableEmergencyFunding = co["enableEmergencyFunding"].get<bool>();
             }
             if (co.contains("fundingSources") && co["fundingSources"].is_array()) {
                 m_collateralConfig.fundingSources.clear();
                 for (auto& source : co["fundingSources"]) {
                     m_collateralConfig.fundingSources.push_back(source.get<std::string>());
                 }
             }
             if (co.contains("maxFundingAmount")) {
                 m_collateralConfig.maxFundingAmount = co["maxFundingAmount"].get<double>();
             }
         }
         
         // Load logging config
         if (j.contains("logging")) {
             auto& lg = j["logging"];
             if (lg.contains("logLevel")) {
                 m_loggingConfig.logLevel = lg["logLevel"].get<std::string>();
             }
             if (lg.contains("logDirectory")) {
                 m_loggingConfig.logDirectory = lg["logDirectory"].get<std::string>();
             }
             if (lg.contains("enableDetailedLogs")) {
                 m_loggingConfig.enableDetailedLogs = lg["enableDetailedLogs"].get<bool>();
             }
             if (lg.contains("logToConsole")) {
                 m_loggingConfig.logToConsole = lg["logToConsole"].get<bool>();
             }
             if (lg.contains("logToFile")) {
                 m_loggingConfig.logToFile = lg["logToFile"].get<bool>();
             }
             if (lg.contains("logRetentionDays")) {
                 m_loggingConfig.logRetentionPeriod = std::chrono::days(lg["logRetentionDays"].get<int>());
             }
         }
         
         // Validate the loaded configuration
         if (!validate()) {
             std::cerr << "Configuration validation failed" << std::endl;
             return false;
         }
         
         return true;
     } catch (const std::exception& e) {
         std::cerr << "Exception while parsing JSON configuration: " << e.what() << std::endl;
         return false;
     }
 }
 
 // Save configuration to file
 bool RiskConfig::saveToFile(const std::string& configFilePath) const {
     try {
         // Create JSON object
         json j;
         
         // Position limit config
         j["positionLimit"] = {
             {"assetLimits", m_positionLimitConfig.assetLimits},
             {"strategyLimits", m_positionLimitConfig.strategyLimits},
             {"exchangeLimits", m_positionLimitConfig.exchangeLimits},
             {"defaultLimit", m_positionLimitConfig.defaultLimit},
             {"enforceStrictly", m_positionLimitConfig.enforceStrictly}
         };
         
         // Exposure limit config
         j["exposureLimit"] = {
             {"maxTotalExposure", m_exposureLimitConfig.maxTotalExposure},
             {"maxAssetExposure", m_exposureLimitConfig.maxAssetExposure},
             {"maxStrategyExposure", m_exposureLimitConfig.maxStrategyExposure},
             {"maxExchangeExposure", m_exposureLimitConfig.maxExchangeExposure},
             {"volatilityMultiplier", m_exposureLimitConfig.volatilityMultiplier},
             {"includeUnrealizedPnl", m_exposureLimitConfig.includeUnrealizedPnl}
         };
         
         // Circuit breaker config
         j["circuitBreaker"] = {
             {"warningThresholds", m_circuitBreakerConfig.warningThresholds},
             {"criticalThresholds", m_circuitBreakerConfig.criticalThresholds},
             {"coolingPeriod", m_circuitBreakerConfig.coolingPeriod.count()},
             {"warmupPeriod", m_circuitBreakerConfig.warmupPeriod.count()},
             {"autoResume", m_circuitBreakerConfig.autoResume},
             {"maxTriggersPerDay", m_circuitBreakerConfig.maxTriggersPerDay}
         };
         
         // Stop loss config
         j["stopLoss"] = {
             {"maxLossPercentage", m_stopLossConfig.maxLossPercentage},
             {"trailingStopDistance", m_stopLossConfig.trailingStopDistance},
             {"enableTrailingStop", m_stopLossConfig.enableTrailingStop},
             {"forceLiquidation", m_stopLossConfig.forceLiquidation},
             {"liquidationDelay", m_stopLossConfig.liquidationDelay.count()}
         };
         
         // Drawdown config
         j["drawdown"] = {
             {"warningThreshold", m_drawdownConfig.warningThreshold},
             {"criticalThreshold", m_drawdownConfig.criticalThreshold},
             {"lookbackPeriodHours", m_drawdownConfig.lookbackPeriod.count()},
             {"enableAutoRiskReduction", m_drawdownConfig.enableAutoRiskReduction},
             {"riskReductionFactor", m_drawdownConfig.riskReductionFactor}
         };
         
         // Fat finger config
         j["fatFinger"] = {
             {"maxPriceDeviation", m_fatFingerConfig.maxPriceDeviation},
             {"maxOrderSize", m_fatFingerConfig.maxOrderSize},
             {"typicalOrderSize", m_fatFingerConfig.typicalOrderSize},
             {"notionalValueLimit", m_fatFingerConfig.notionalValueLimit},
             {"enforceAdditionalVerification", m_fatFingerConfig.enforceAdditionalVerification}
         };
         
         // Order validation config
         json minOrderSizes;
         for (const auto& [exchange, pairs] : m_orderValidationConfig.minOrderSizes) {
             json pairSizes;
             for (const auto& [pair, size] : pairs) {
                 pairSizes[pair.symbol] = size;
             }
             minOrderSizes[exchange.id] = pairSizes;
         }
         
         j["orderValidation"] = {
             {"validateMinOrderSize", m_orderValidationConfig.validateMinOrderSize},
             {"validatePriceRange", m_orderValidationConfig.validatePriceRange},
             {"minOrderSizes", minOrderSizes},
             {"maxPriceDeviationPercent", m_orderValidationConfig.maxPriceDeviationPercent},
             {"enforceSymbolWhitelist", m_orderValidationConfig.enforceSymbolWhitelist},
             {"symbolWhitelist", m_orderValidationConfig.symbolWhitelist}
         };
         
         // Balance reconciliation config
         j["balanceReconciliation"] = {
             {"reconciliationFrequencyMinutes", m_balanceReconciliationConfig.reconciliationFrequency.count()},
             {"maxDiscrepancyPercent", m_balanceReconciliationConfig.maxDiscrepancyPercent},
             {"autoCorrectDiscrepancies", m_balanceReconciliationConfig.autoCorrectDiscrepancies},
             {"alertOnDiscrepancy", m_balanceReconciliationConfig.alertOnDiscrepancy},
             {"maxConsecutiveFailures", m_balanceReconciliationConfig.maxConsecutiveFailures}
         };
         
         // Rebalancing config
         j["rebalancing"] = {
             {"rebalanceThreshold", m_rebalancingConfig.rebalanceThreshold},
             {"checkFrequencyHours", m_rebalancingConfig.checkFrequency.count()},
             {"autoRebalance", m_rebalancingConfig.autoRebalance},
             {"preferLowFeeExchanges", m_rebalancingConfig.preferLowFeeExchanges},
             {"minimumTimeBetweenRebalancesSeconds", m_rebalancingConfig.minimumTimeBetweenRebalances.count()}
         };
         
         // Collateral config
         j["collateral"] = {
             {"minFreeCollateralRatio", m_collateralConfig.minFreeCollateralRatio},
             {"targetFreeCollateralRatio", m_collateralConfig.targetFreeCollateralRatio},
             {"enableEmergencyFunding", m_collateralConfig.enableEmergencyFunding},
             {"fundingSources", m_collateralConfig.fundingSources},
             {"maxFundingAmount", m_collateralConfig.maxFundingAmount}
         };
         
         // Logging config
         j["logging"] = {
             {"logLevel", m_loggingConfig.logLevel},
             {"logDirectory", m_loggingConfig.logDirectory},
             {"enableDetailedLogs", m_loggingConfig.enableDetailedLogs},
             {"logToConsole", m_loggingConfig.logToConsole},
             {"logToFile", m_loggingConfig.logToFile},
             {"logRetentionDays", m_loggingConfig.logRetentionPeriod.count()}
         };
         
         // Write to file
         std::ofstream file(configFilePath);
         if (!file.is_open()) {
             std::cerr << "Failed to open file for writing: " << configFilePath << std::endl;
             return false;
         }
         
         file << j.dump(4); // Pretty print with 4-space indentation
         file.close();
         
         return true;
     } catch (const std::exception& e) {
         std::cerr << "Exception while saving configuration: " << e.what() << std::endl;
         return false;
     }
 }
 
 // Validate configuration values
 bool RiskConfig::validate() const {
     // Validate position limits
     if (m_positionLimitConfig.defaultLimit <= 0) {
         std::cerr << "Invalid default position limit: must be positive" << std::endl;
         return false;
     }
     
     // Validate exposure limits
     if (m_exposureLimitConfig.maxTotalExposure <= 0 ||
         m_exposureLimitConfig.maxAssetExposure <= 0 ||
         m_exposureLimitConfig.maxStrategyExposure <= 0 ||
         m_exposureLimitConfig.maxExchangeExposure <= 0) {
         std::cerr << "Invalid exposure limits: all limits must be positive" << std::endl;
         return false;
     }
     
     if (m_exposureLimitConfig.volatilityMultiplier <= 0) {
         std::cerr << "Invalid volatility multiplier: must be positive" << std::endl;
         return false;
     }
     
     // Validate circuit breaker thresholds
     for (const auto& [metric, threshold] : m_circuitBreakerConfig.warningThresholds) {
         if (threshold <= 0) {
             std::cerr << "Invalid warning threshold for " << metric << ": must be positive" << std::endl;
             return false;
         }
     }
     
     for (const auto& [metric, threshold] : m_circuitBreakerConfig.criticalThresholds) {
         if (threshold <= 0) {
             std::cerr << "Invalid critical threshold for " << metric << ": must be positive" << std::endl;
             return false;
         }
     }
     
     // Make sure critical thresholds are higher than warning thresholds
     for (const auto& [metric, warnThreshold] : m_circuitBreakerConfig.warningThresholds) {
         auto criticalIt = m_circuitBreakerConfig.criticalThresholds.find(metric);
         if (criticalIt != m_circuitBreakerConfig.criticalThresholds.end()) {
             if (criticalIt->second <= warnThreshold) {
                 std::cerr << "Invalid thresholds for " << metric
                           << ": critical threshold must be greater than warning threshold" << std::endl;
                 return false;
             }
         }
     }
     
     // Validate stop loss config
     if (m_stopLossConfig.maxLossPercentage <= 0 || m_stopLossConfig.maxLossPercentage > 1.0) {
         std::cerr << "Invalid max loss percentage: must be between 0 and 1" << std::endl;
         return false;
     }
     
     if (m_stopLossConfig.trailingStopDistance <= 0 || m_stopLossConfig.trailingStopDistance > 1.0) {
         std::cerr << "Invalid trailing stop distance: must be between 0 and 1" << std::endl;
         return false;
     }
     
     // Validate drawdown config
     if (m_drawdownConfig.warningThreshold <= 0 || m_drawdownConfig.warningThreshold > 1.0) {
         std::cerr << "Invalid drawdown warning threshold: must be between 0 and 1" << std::endl;
         return false;
     }
     
     if (m_drawdownConfig.criticalThreshold <= 0 || m_drawdownConfig.criticalThreshold > 1.0) {
         std::cerr << "Invalid drawdown critical threshold: must be between 0 and 1" << std::endl;
         return false;
     }
     
     if (m_drawdownConfig.criticalThreshold <= m_drawdownConfig.warningThreshold) {
         std::cerr << "Invalid drawdown thresholds: critical threshold must be greater than warning threshold" << std::endl;
         return false;
     }
     
     if (m_drawdownConfig.lookbackPeriod.count() <= 0) {
         std::cerr << "Invalid drawdown lookback period: must be positive" << std::endl;
         return false;
     }
     
     if (m_drawdownConfig.riskReductionFactor <= 0 || m_drawdownConfig.riskReductionFactor > 1.0) {
         std::cerr << "Invalid risk reduction factor: must be between 0 and 1" << std::endl;
         return false;
     }
     
     // Validate fat finger config
     if (m_fatFingerConfig.maxPriceDeviation <= 0) {
         std::cerr << "Invalid max price deviation: must be positive" << std::endl;
         return false;
     }
     
     if (m_fatFingerConfig.maxOrderSize <= 0) {
         std::cerr << "Invalid max order size: must be positive" << std::endl;
         return false;
     }
     
     if (m_fatFingerConfig.typicalOrderSize <= 0) {
         std::cerr << "Invalid typical order size: must be positive" << std::endl;
         return false;
     }
     
     if (m_fatFingerConfig.notionalValueLimit <= 0) {
         std::cerr << "Invalid notional value limit: must be positive" << std::endl;
         return false;
     }
     
     // Validate order validation config
     if (m_orderValidationConfig.maxPriceDeviationPercent <= 0) {
         std::cerr << "Invalid max price deviation percent: must be positive" << std::endl;
         return false;
     }
     
     // Validate balance reconciliation config
     if (m_balanceReconciliationConfig.reconciliationFrequency.count() <= 0) {
         std::cerr << "Invalid reconciliation frequency: must be positive" << std::endl;
         return false;
     }
     
     if (m_balanceReconciliationConfig.maxDiscrepancyPercent <= 0) {
         std::cerr << "Invalid max discrepancy percent: must be positive" << std::endl;
         return false;
     }
     
     if (m_balanceReconciliationConfig.maxConsecutiveFailures <= 0) {
         std::cerr << "Invalid max consecutive failures: must be positive" << std::endl;
         return false;
     }
     
     // Validate rebalancing config
     if (m_rebalancingConfig.rebalanceThreshold <= 0) {
         std::cerr << "Invalid rebalance threshold: must be positive" << std::endl;
         return false;
     }
     
     if (m_rebalancingConfig.checkFrequency.count() <= 0) {
         std::cerr << "Invalid rebalancing check frequency: must be positive" << std::endl;
         return false;
     }
     
     if (m_rebalancingConfig.minimumTimeBetweenRebalances.count() <= 0) {
         std::cerr << "Invalid minimum time between rebalances: must be positive" << std::endl;
         return false;
     }
     
     // Validate collateral config
     if (m_collateralConfig.minFreeCollateralRatio < 0 || m_collateralConfig.minFreeCollateralRatio > 1.0) {
         std::cerr << "Invalid min free collateral ratio: must be between 0 and 1" << std::endl;
         return false;
     }
     
     if (m_collateralConfig.targetFreeCollateralRatio < 0 || m_collateralConfig.targetFreeCollateralRatio > 1.0) {
         std::cerr << "Invalid target free collateral ratio: must be between 0 and 1" << std::endl;
         return false;
     }
     
     if (m_collateralConfig.targetFreeCollateralRatio < m_collateralConfig.minFreeCollateralRatio) {
         std::cerr << "Invalid collateral ratios: target ratio must be greater than or equal to min ratio" << std::endl;
         return false;
     }
     
     if (m_collateralConfig.maxFundingAmount <= 0) {
         std::cerr << "Invalid max funding amount: must be positive" << std::endl;
         return false;
     }
     
     // All validations passed
     return true;
 }
 
 // Getters and setters for configuration sections
 const PositionLimitConfig& RiskConfig::getPositionLimitConfig() const {
     return m_positionLimitConfig;
 }
 
 void RiskConfig::setPositionLimitConfig(const PositionLimitConfig& config) {
     m_positionLimitConfig = config;
 }
 
 const ExposureLimitConfig& RiskConfig::getExposureLimitConfig() const {
     return m_exposureLimitConfig;
 }
 
 void RiskConfig::setExposureLimitConfig(const ExposureLimitConfig& config) {
     m_exposureLimitConfig = config;
 }
 
 const CircuitBreakerConfig& RiskConfig::getCircuitBreakerConfig() const {
     return m_circuitBreakerConfig;
 }
 
 void RiskConfig::setCircuitBreakerConfig(const CircuitBreakerConfig& config) {
     m_circuitBreakerConfig = config;
 }
 
 const StopLossConfig& RiskConfig::getStopLossConfig() const {
     return m_stopLossConfig;
 }
 
 void RiskConfig::setStopLossConfig(const StopLossConfig& config) {
     m_stopLossConfig = config;
 }
 
 const DrawdownConfig& RiskConfig::getDrawdownConfig() const {
     return m_drawdownConfig;
 }
 
 void RiskConfig::setDrawdownConfig(const DrawdownConfig& config) {
     m_drawdownConfig = config;
 }
 
 const FatFingerConfig& RiskConfig::getFatFingerConfig() const {
     return m_fatFingerConfig;
 }
 
 void RiskConfig::setFatFingerConfig(const FatFingerConfig& config) {
     m_fatFingerConfig = config;
 }
 
 const OrderValidationConfig& RiskConfig::getOrderValidationConfig() const {
     return m_orderValidationConfig;
 }
 
 void RiskConfig::setOrderValidationConfig(const OrderValidationConfig& config) {
     m_orderValidationConfig = config;
 }
 
 const BalanceReconciliationConfig& RiskConfig::getBalanceReconciliationConfig() const {
     return m_balanceReconciliationConfig;
 }
 
 void RiskConfig::setBalanceReconciliationConfig(const BalanceReconciliationConfig& config) {
     m_balanceReconciliationConfig = config;
 }
 
 const RebalancingConfig& RiskConfig::getRebalancingConfig() const {
     return m_rebalancingConfig;
 }
 
 void RiskConfig::setRebalancingConfig(const RebalancingConfig& config) {
     m_rebalancingConfig = config;
 }
 
 const CollateralConfig& RiskConfig::getCollateralConfig() const {
     return m_collateralConfig;
 }
 
 void RiskConfig::setCollateralConfig(const CollateralConfig& config) {
     m_collateralConfig = config;
 }
 
 const RiskLoggingConfig& RiskConfig::getLoggingConfig() const {
     return m_loggingConfig;
 }
 
 void RiskConfig::setLoggingConfig(const RiskLoggingConfig& config) {
     m_loggingConfig = config;
 }
 
 } // namespace risk
} // namespace order_imbalance_system