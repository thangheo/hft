#!/bin/bash

# build.sh - Script to build the HFT Crypto Trading System

# Exit immediately if a command exits with a non-zero status
set -e

echo "Starting build process..."

# Create build directory if it doesn't exist
mkdir -p ../build
cd ../build

# Run CMake to configure the project
cmake ..

# Build the project using all available cores
make -j$(nproc)

echo "Build completed successfully."
