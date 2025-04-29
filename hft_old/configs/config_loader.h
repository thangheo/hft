#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <map>
#include <yaml-cpp/yaml.h>

class ConfigLoader {
public:
    ConfigLoader(const std::string& config_directory);
    ~ConfigLoader();

    bool loadExchangeConfig(const std::string& exchange_name);
    bool loadRiskConfig();
    bool loadStrategyConfig(const std::string& strategy_name);

    // Accessors
    YAML::Node getExchangeConfig(const std::string& exchange_name) const;
    YAML::Node getRiskConfig() const;
    YAML::Node getStrategyConfig(const std::string& strategy_name) const;

private:
    std::string config_directory_;

    std::map<std::string, YAML::Node> exchange_configs_;
    YAML::Node risk_config_;
    std::map<std::string, YAML::Node> strategy_configs_;

    bool loadConfigFile(const std::string& file_path, YAML::Node& node);
};

#endif // CONFIG_LOADER_H
