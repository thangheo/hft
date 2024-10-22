#include "config_loader.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

ConfigLoader::ConfigLoader(const std::string& config_directory)
    : config_directory_(config_directory) {
}

ConfigLoader::~ConfigLoader() {
}

// bool ConfigLoader::loadExchangeConfig(const std::string& exchange_name) {
//     std::string file_path = config_directory_ + "/exchange_configs/" + exchange_name + ".yml";
//     YAML::Node config;
//     if (loadConfigFile(file_path, config)) {
//         exchange_configs_[exchange_name] = config;
//         return true;
//     }
//     return false;
// }
bool ConfigLoader::loadExchangeConfig(const std::string& exchange_name) {
    std::string file_path = config_directory_ + "/exchange_configs/" + exchange_name + ".yml";
    YAML::Node config;
    if (loadConfigFile(file_path, config)) {
        // Validate required fields
        if (!config["exchange_name"] || !config["base_url"] || !config["symbols"]) {
            std::cerr << "Missing required fields in exchange config: " << exchange_name << std::endl;
            return false;
        }
        exchange_configs_[exchange_name] = config;
        return true;
    }
    return false;
}

bool ConfigLoader::loadRiskConfig() {
    std::string file_path = config_directory_ + "/risk_configs/risk_config.yml";
    return loadConfigFile(file_path, risk_config_);
}

bool ConfigLoader::loadStrategyConfig(const std::string& strategy_name) {
    std::string file_path = config_directory_ + "/strategy_configs/" + strategy_name + ".yml";
    YAML::Node config;
    if (loadConfigFile(file_path, config)) {
        strategy_configs_[strategy_name] = config;
        return true;
    }
    return false;
}

YAML::Node ConfigLoader::getExchangeConfig(const std::string& exchange_name) const {
    return exchange_configs_.at(exchange_name);
}

YAML::Node ConfigLoader::getRiskConfig() const {
    return risk_config_;
}

YAML::Node ConfigLoader::getStrategyConfig(const std::string& strategy_name) const {
    return strategy_configs_.at(strategy_name);
}

bool ConfigLoader::loadConfigFile(const std::string& file_path, YAML::Node& node) {
    try {
        node = YAML::LoadFile(file_path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << file_path << "\n"
                  << "Exception: " << e.what() << std::endl;
        return false;
    }
}
