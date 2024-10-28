#!/bin/bash

# build.sh - Script to build the HFT Crypto Trading System

# Exit immediately if a command exits with a non-zero status
set -e

echo "Starting build process..."
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

# set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
# set(CMAKE_INSTALL_RPATH "/usr/lib/x86_64-linux-gnu")
#generate protocbuf files
#conda deactivate
cd ../
protoc --cpp_out=src/data_storage/serialization/ proto/order_data.proto
protoc --cpp_out=src/data_storage/serialization/ proto/market_data.proto 
mv src/data_storage/serialization/proto/* src/data_storage/serialization/
cd scripts
# cd "$(dirname "$0")"
# Create build directory if it doesn't exist
mkdir -p ../build
cd ../build

# Run CMake to configure the project
cmake ..

# Build the project using all available cores
make -j$(nproc)

echo "Build completed successfully."
