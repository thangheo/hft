#!/bin/bash

# start_trading.sh - Script to start the HFT Crypto Trading Application

# Exit immediately if a command exits with a non-zero status
set -e

echo "Starting HFT Crypto Trading Application..."

# Run the application with necessary configurations
/usr/local/bin/hft_trading_system --config /etc/hft_trading_system/main_config.yaml &

echo "HFT Crypto Trading Application started."
