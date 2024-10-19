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

#gtest 
#or
git submodule add https://github.com/google/googletest.git
git submodule update --init
#redis-plus-plus and hiredis
git clone https://github.com/redis/hiredis.git
cd hiredis
 
 
 # Install other dependencies as needed
# For example, install InfluxDB client library for C++ if using InfluxDB

echo "Environment setup completed successfully."
sudo apt-get install libstdc++6

git submodule add https://github.com/boostorg/boost.git third_party/boost
git submodule add https://github.com/jupp0r/prometheus-cpp.git third_party/prometheus-cpp
git submodule add https://github.com/gabime/spdlog.git third_party/spdlog
git submodule add https://github.com/timescale/timescaledb.git third_party/timescaledb
git submodule add https://github.com/sewenew/redis-plus-plus.git third_party/redis-plus-plus
git submodule add https://github.com/redis/hiredis.git third_party/hiredis
git submodule add https://github.com/nlohmann/json.git third_party/nlohmann_json
git submodule add https://github.com/protocolbuffers/protobuf.git third_party/protobuf
git submodule add https://github.com/yhirose/cpp-httplib.git third_party/cpp-httplib
git submodule add git submodule add  https://github.com/jtv/libpqxx.git third_party/pqxx


#remember to check out version for protobuf and protoc
git submodule update --init --recursive

sudo apt install timescaledb-tools
sudo timescaledb-tune

CREATE DATABASE trading_db;
\c trading_db;
CREATE EXTENSION IF NOT EXISTS timescaledb;
CREATE TABLE market_data (
    id SERIAL,
    exchange TEXT NOT NULL,
    symbol TEXT NOT NULL,
    timestamp TIMESTAMP WITH TIME ZONE NOT NULL,
    bids JSONB,
    asks JSONB,
    PRIMARY KEY (id, timestamp)
);

SELECT create_hypertable('market_data', 'timestamp');

CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    order_id TEXT NOT NULL,
    client_order_id TEXT,
    symbol TEXT NOT NULL,
    side TEXT,
    price DOUBLE PRECISION,
    quantity DOUBLE PRECISION,
    status TEXT,
    timestamp TIMESTAMP WITH TIME ZONE NOT NULL
);


#clone httplib
#
# cd third_party
#git clone git@github.com:yhirose/cpp-httplib.git
#cd cpp-httplib
#mkdir build
#cd build
#cmake ..
#make
#sudo make install

#install prometheus-cpp
# cd third_party
# git clone git@github.com:jupp0r/prometheus-cpp.git
# cd prometheus-cpp
# git submodule init
# git submodule update
# mkdir _build
# cd _build
# cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF
# make
# sudo make install
