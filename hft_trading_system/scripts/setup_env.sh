#!/bin/bash

# setup_env.sh - Script to set up the development environment for HFT Crypto Trading System

# Exit immediately if a command exits with a non-zero status
set -e

echo "Starting environment setup..."

# Update package lists
sudo apt-get update

# Install essential build tools
sudo apt-get install -y build-essential cmake git

# Install Boost Libraries
sudo apt-get install -y libboost-all-dev

# Install Asio standalone (since C++11, Asio can be used without Boost)
sudo apt-get install -y asio-dev
#install glibc 6
sudo apt-get install -y libstdc++6
# Install SSL libraries
sudo apt-get install -y libssl-dev

# Install Redis (if used for in-memory data storage)
sudo apt-get install -y redis-server libhiredis-dev

# Install Google Test for unit testing
sudo apt-get install -y libgtest-dev

# Build and install Google Test (some distributions require manual build)
cd /usr/src/gtest
sudo cmake .
sudo make
sudo cp lib/*.a /usr/lib
cd -

# Install spdlog for logging
sudo apt-get install -y libspdlog-dev

# Install Protocol Buffers for serialization
sudo apt-get install -y protobuf-compiler libprotobuf-dev

# Install other dependencies as needed
# For example, install InfluxDB client library for C++ if using InfluxDB

echo "Environment setup completed successfully."
sudo apt-get install libstdc++6
# sudo add-apt-repository ppa:ubuntu-toolchain-r/test 
# sudo apt-get update
# sudo apt-get upgrade
# sudo apt-get dist-upgrade