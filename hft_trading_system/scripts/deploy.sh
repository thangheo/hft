#!/bin/bash

# deploy.sh - Script to deploy the HFT Crypto Trading System

# Exit immediately if a command exits with a non-zero status
set -e

echo "Starting deployment..."

# Define paths
BUILD_DIR="./build"
TARGET_BIN="/usr/local/bin/hft_trading_system"
CONFIG_DIR="/etc/hft_trading_system"

# Stop running services (if any)
sudo systemctl stop hft_trading.service || true

# Copy binaries to /usr/local/bin
sudo cp $BUILD_DIR/hft_trading_system $TARGET_BIN

# Copy configuration files
sudo mkdir -p $CONFIG_DIR
sudo cp -r ./configs/* $CONFIG_DIR/

# Set appropriate permissions
sudo chmod +x $TARGET_BIN

# Start services
sudo systemctl start hft_trading.service

echo "Deployment completed successfully."
