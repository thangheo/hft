#!/bin/bash

# Define the base directory
base_dir="hft_trading_system"

# Create the base directory and subdirectories
mkdir -p $base_dir/{docs/{architecture,api,user_guides},configs/{exchange_configs,strategy_configs,risk_configs},include/{common,market_data,strategy,oms,risk_management,connectivity,data_storage,monitoring},src/{common,market_data/adapters,market_data/protocols,strategy/strategies,strategy/backtesting/test_data,oms/order_execution,oms/order_tracking,oms/exchange_interfaces,risk_management/pre_trade_checks,risk_management/position_monitoring,risk_management/risk_rules,connectivity/protocols,data_storage/real_time_data,data_storage/historical_data,data_storage/serialization,monitoring/logging,monitoring/metrics,monitoring/alerts},tests/{unit_tests,integration_tests},scripts/utils,third_party/{boost,spdlog},resources/ssl_certificates,resources/sample_configs}

# Create sample files
touch $base_dir/{CMakeLists.txt,README.md,LICENSE}
touch $base_dir/src/main.cpp
touch $base_dir/scripts/{deploy.sh,setup_env.sh}
touch $base_dir/scripts/utils/{data_cleanup.py,log_rotate.sh}
