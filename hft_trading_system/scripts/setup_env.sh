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
sudo apt-get install libgtest-dev
cd /usr/src/gtest
sudo cmake .
sudo make
sudo cp lib/*.a /usr/lib
#or
git submodule add https://github.com/google/googletest.git
git submodule update --init
#redis-plus-plus and hiredis
git clone https://github.com/redis/hiredis.git
cd hiredis

# sudo apt-get install libhiredis-dev
git clone https://github.com/sewenew/redis-plus-plus.git
cd redis-plus-plus
mkdir build && cd build
cmake .. -DREDIS_PLUS_PLUS_BUILD_SHARED=ON
make -j2
sudo make install

 
 # Install other dependencies as needed
# For example, install InfluxDB client library for C++ if using InfluxDB

echo "Environment setup completed successfully."
sudo apt-get install libstdc++6
# sudo add-apt-repository ppa:ubuntu-toolchain-r/test 
# sudo apt-get update
# sudo apt-get upgrade
# sudo apt-get dist-upgrade
#install postgresql-16 and TimescaleDB

# sudo apt install gnupg postgresql-common apt-transport-https lsb-release wget
# sudo /usr/share/postgresql-common/pgdg/apt.postgresql.org.sh
# sudo apt install postgresql-server-dev-16
# echo "deb https://packagecloud.io/timescale/timescaledb/debian/ $(lsb_release -c -s) main" | sudo tee /etc/apt/sources.list.d/timescaledb.list
# wget --quiet -O - https://packagecloud.io/timescale/timescaledb/gpgkey | sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/timescaledb.gpg
# sudo apt update
# sudo apt install timescaledb-2-postgresql-16 postgresql-client-16

# sudo apt install postgresql-server-dev-15
# git clone https://github.com/timescale/timescaledb.git
# cd timescaledb
# git checkout 2.15.3
# ./bootstrap
# cd build && make
# sudo make install
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
