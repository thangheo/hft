#!/usr/bin/env python3
"""
Script to generate the order imbalance trading system project structure.
"""

import os
import json
from pathlib import Path

def create_file(path, content=""):
    """Create a file with optional content."""
    with open(path, 'w') as f:
        f.write(content)
    print(f"Created: {path}")

def create_directory(path):
    """Create a directory if it doesn't exist."""
    if not os.path.exists(path):
        os.makedirs(path)

def main():
    # Base directory for the project
    base_dir = "order_imbalance_system"
    create_directory(base_dir)
    
    # Main CMakeLists.txt
    cmake_content = """cmake_minimum_required(VERSION 3.15)
project(OrderImbalanceSystem VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build the tests" ON)
option(USE_SANITIZERS "Use address and undefined behavior sanitizers" OFF)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Find packages
find_package(Threads REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(Flatbuffers REQUIRED)
find_package(Boost REQUIRED COMPONENTS system program_options)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
  if(USE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
  endif()
endif()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(libs)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
"""
    create_file(os.path.join(base_dir, "CMakeLists.txt"), cmake_content)
    
    # Create directory structure
    directories = [
        "config",
        "docker",
        "docker/kubernetes",
        "docs",
        "docs/api",
        "include/common",
        "include/data_ingestion",
        "include/strategy",
        "include/execution",
        "include/risk",
        "libs",
        "libs/external",
        "python/backtest",
        "python/research",
        "python/visualization",
        "scripts",
        "src/common",
        "src/data_ingestion",
        "src/strategy",
        "src/execution",
        "src/risk",
        "tests/common",
        "tests/data_ingestion",
        "tests/strategy",
        "tests/execution",
        "tests/risk"
    ]
    
    for directory in directories:
        create_directory(os.path.join(base_dir, directory))
    
    # Create configuration files
    config_files = {
        "exchanges.json": json.dumps({
            "binance": {
                "api_key": "",
                "api_secret": "",
                "websocket_url": "wss://stream.binance.com:9443/ws",
                "rest_api_url": "https://api.binance.com"
            },
            "coinbase": {
                "api_key": "",
                "api_secret": "",
                "websocket_url": "wss://ws-feed.pro.coinbase.com",
                "rest_api_url": "https://api.pro.coinbase.com"
            }
        }, indent=2),
        
        "strategy_params.json": json.dumps({
            "thresholds": {
                "min_imbalance_ratio": 2.0,
                "confidence_threshold": 0.7
            },
            "timeframes": {
                "analysis_window_ms": 2000,
                "signal_ttl_ms": 500
            }
        }, indent=2),
        
        "risk_limits.json": json.dumps({
            "position_limits": {
                "BTC": 1.0,
                "ETH": 10.0
            },
            "max_drawdown_percent": 2.0,
            "max_open_orders": 5
        }, indent=2),
        
        "system_config.json": json.dumps({
            "log_level": "info",
            "data_retention_days": 7,
            "performance_monitor_interval_ms": 1000
        }, indent=2)
    }
    
    for filename, content in config_files.items():
        create_file(os.path.join(base_dir, "config", filename), content)
    
    # Create Docker files
    docker_files = {
        "Dockerfile.core": """FROM debian:bullseye-slim as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \\
    build-essential \\
    cmake \\
    git \\
    libzmq3-dev \\
    libboost-all-dev \\
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Build the project
RUN mkdir -p build && cd build && \\
    cmake .. && \\
    make -j$(nproc)

# Runtime image
FROM debian:bullseye-slim

# Install runtime dependencies
RUN apt-get update && apt-get install -y \\
    libzmq3-dev \\
    libboost-system1.74.0 \\
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/src/order_imbalance_system .
COPY --from=builder /app/config ./config

ENTRYPOINT ["./order_imbalance_system"]
""",
        
        "Dockerfile.python": """FROM python:3.9-slim

WORKDIR /app

# Install dependencies
COPY python/requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy Python code
COPY python/ ./python/
COPY config/ ./config/

ENTRYPOINT ["python", "-m", "python.backtest.simulator"]
""",
        
        "docker-compose.yml": """version: '3.8'

services:
  core:
    build:
      context: ..
      dockerfile: docker/Dockerfile.core
    depends_on:
      - influxdb
      - postgres
    environment:
      - POSTGRES_HOST=postgres
      - INFLUXDB_HOST=influxdb
    volumes:
      - ../config:/app/config
    networks:
      - trading_network

  python:
    build:
      context: ..
      dockerfile: docker/Dockerfile.python
    depends_on:
      - influxdb
    volumes:
      - ../python:/app/python
      - ../config:/app/config
    networks:
      - trading_network

  influxdb:
    image: influxdb:2.0
    ports:
      - "8086:8086"
    volumes:
      - influxdb_data:/var/lib/influxdb2
    environment:
      - DOCKER_INFLUXDB_INIT_MODE=setup
      - DOCKER_INFLUXDB_INIT_USERNAME=admin
      - DOCKER_INFLUXDB_INIT_PASSWORD=adminpassword
      - DOCKER_INFLUXDB_INIT_ORG=trading
      - DOCKER_INFLUXDB_INIT_BUCKET=marketdata
    networks:
      - trading_network

  postgres:
    image: postgres:13
    ports:
      - "5432:5432"
    volumes:
      - postgres_data:/var/lib/postgresql/data
    environment:
      - POSTGRES_PASSWORD=password
      - POSTGRES_USER=trading
      - POSTGRES_DB=orderstate
    networks:
      - trading_network

  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    depends_on:
      - influxdb
      - prometheus
    volumes:
      - grafana_data:/var/lib/grafana
    networks:
      - trading_network

  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9090:9090"
    volumes:
      - ../config/prometheus.yml:/etc/prometheus/prometheus.yml
      - prometheus_data:/prometheus
    networks:
      - trading_network

networks:
  trading_network:

volumes:
  influxdb_data:
  postgres_data:
  grafana_data:
  prometheus_data:
"""
    }
    
    for filename, content in docker_files.items():
        create_file(os.path.join(base_dir, "docker", filename), content)
    
    # Create Kubernetes files
    k8s_files = {
        "deployment.yaml": """apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-imbalance-system
  labels:
    app: order-imbalance-system
spec:
  replicas: 3
  selector:
    matchLabels:
      app: order-imbalance-system
  template:
    metadata:
      labels:
        app: order-imbalance-system
    spec:
      containers:
      - name: core
        image: order-imbalance-system:latest
        imagePullPolicy: IfNotPresent
        resources:
          limits:
            cpu: "2"
            memory: "2Gi"
          requests:
            cpu: "1"
            memory: "1Gi"
        env:
        - name: POSTGRES_HOST
          value: postgres-service
        - name: INFLUXDB_HOST
          value: influxdb-service
        volumeMounts:
        - name: config-volume
          mountPath: /app/config
      volumes:
      - name: config-volume
        configMap:
          name: trading-config
""",
        
        "service.yaml": """apiVersion: v1
kind: Service
metadata:
  name: order-imbalance-service
spec:
  selector:
    app: order-imbalance-system
  ports:
  - port: 8080
    targetPort: 8080
  type: ClusterIP
"""
    }
    
    for filename, content in k8s_files.items():
        create_file(os.path.join(base_dir, "docker/kubernetes", filename), content)
    
    # Create architecture.md doc
    arch_doc = """# Order Imbalance Trading System Architecture

## Overview

This system implements a trading strategy based on order book imbalances. It detects significant 
differences between buy and sell orders which may indicate short-term price movements.

## Components

### Data Ingestion Layer
- WebSocket connections to exchanges
- Order book reconstruction
- Data normalization

### Strategy Engine
- Analyzes order book data
- Detects imbalances
- Generates trading signals

### Execution Engine
- Processes signals
- Places orders on exchanges
- Manages order lifecycles

### Risk Management System
- Pre-trade risk filters
- Position monitoring
- Circuit breakers

### Performance Monitoring
- Latency tracking
- Strategy metrics
- Alerts and notifications

## Data Flow

1. Market data is collected from exchange WebSockets
2. Order books are reconstructed and normalized
3. The strategy engine analyzes for imbalances
4. Trading signals are generated
5. Signals pass through risk filters
6. Orders are executed on exchanges
7. Results feed back for performance monitoring

## Technology Stack

- C++17 for performance-critical components
- Python for research and visualization
- ZeroMQ for internal messaging
- InfluxDB for time-series data
- PostgreSQL for order state and configuration
"""
    create_file(os.path.join(base_dir, "docs/architecture.md"), arch_doc)
    
    # Create main.cpp
    main_cpp = """#include <iostream>
#include <thread>
#include <vector>
#include <signal.h>

#include "common/config.hpp"
#include "common/logging.hpp"
#include "data_ingestion/websocket_client.hpp"
#include "data_ingestion/order_book.hpp"
#include "strategy/analyzer.hpp"
#include "strategy/imbalance_detector.hpp"
#include "strategy/signal_generator.hpp"
#include "execution/signal_processor.hpp"
#include "execution/order_service.hpp"
#include "risk/pre_trade_filter.hpp"

// Global flag for graceful shutdown
volatile bool running = true;

void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    // Register signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // Initialize logging
        common::Logging::init();
        
        // Load configuration
        auto config = common::Config::load("config/system_config.json");
        auto exchanges_config = common::Config::load("config/exchanges.json");
        auto strategy_config = common::Config::load("config/strategy_params.json");
        auto risk_config = common::Config::load("config/risk_limits.json");
        
        LOG_INFO("System starting up...");
        
        // Initialize components
        data_ingestion::OrderBook orderBook;
        strategy::Analyzer analyzer(strategy_config);
        strategy::ImbalanceDetector detector(strategy_config);
        strategy::SignalGenerator signalGen(strategy_config);
        risk::PreTradeFilter riskFilter(risk_config);
        execution::SignalProcessor signalProcessor;
        execution::OrderService orderService(exchanges_config);
        
        // Start WebSocket clients for each exchange
        std::vector<std::unique_ptr<data_ingestion::WebsocketClient>> clients;
        for (const auto& exchange : exchanges_config.getExchanges()) {
            auto client = std::make_unique<data_ingestion::WebsocketClient>(
                exchange.name,
                exchange.websocket_url
            );
            client->connect();
            clients.push_back(std::move(client));
        }
        
        LOG_INFO("All components initialized, entering main loop");
        
        // Main loop
        while (running) {
            // Process updates from clients to order book
            orderBook.processUpdates();
            
            // Analyze order book for imbalances
            auto metrics = analyzer.analyze(orderBook);
            
            // Detect significant imbalances
            auto imbalances = detector.detect(metrics);
            
            // Generate trading signals
            if (!imbalances.empty()) {
                auto signals = signalGen.generateSignals(imbalances);
                
                // Process signals through risk filter
                for (const auto& signal : signals) {
                    if (riskFilter.validateSignal(signal)) {
                        // Process valid signals
                        auto order = signalProcessor.processSignal(signal);
                        orderService.placeOrder(order);
                    }
                }
            }
            
            // Small sleep to prevent CPU hogging
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Cleanup
        LOG_INFO("Shutting down gracefully...");
        for (auto& client : clients) {
            client->disconnect();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
"""
    create_file(os.path.join(base_dir, "src/main.cpp"), main_cpp)
    
    # Create a few header files as examples
    header_files = {
        "include/common/types.hpp": """#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace common {

using Timestamp = std::chrono::time_point<std::chrono::system_clock>;

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT
};

struct PriceLevel {
    double price;
    double quantity;
    int order_count;
};

using OrderBook = std::unordered_map<double, PriceLevel>;

struct ImbalanceMetrics {
    double bid_ask_ratio;
    double volume_delta;
    double price_pressure;
    double acceleration;
    Timestamp timestamp;
};

struct TradingSignal {
    std::string instrument;
    OrderSide side;
    double confidence;
    double target_price;
    double stop_price;
    Timestamp expiry;
    Timestamp timestamp;
};

struct Order {
    std::string instrument;
    std::string exchange;
    std::string id;
    OrderSide side;
    OrderType type;
    double quantity;
    double price;
    Timestamp timestamp;
};

} // namespace common
""",
        
        "include/data_ingestion/order_book.hpp": """#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "common/types.hpp"

namespace data_ingestion {

class OrderBook {
public:
    OrderBook();
    ~OrderBook();
    
    // Update the order book with new data
    void update(const std::string& instrument, 
                const std::vector<common::PriceLevel>& bids,
                const std::vector<common::PriceLevel>& asks);
    
    // Process any pending updates
    void processUpdates();
    
    // Get current state of the order book
    std::vector<common::PriceLevel> getBids(const std::string& instrument, size_t depth = 10) const;
    std::vector<common::PriceLevel> getAsks(const std::string& instrument, size_t depth = 10) const;
    
    // Calculate mid price
    double getMidPrice(const std::string& instrument) const;
    
    // Get total volume at specified levels
    double getBidVolume(const std::string& instrument, size_t depth = 10) const;
    double getAskVolume(const std::string& instrument, size_t depth = 10) const;

private:
    struct BookData {
        std::vector<common::PriceLevel> bids;
        std::vector<common::PriceLevel> asks;
        std::mutex mutex;
    };
    
    std::unordered_map<std::string, BookData> books_;
    mutable std::mutex mutex_;
};

} // namespace data_ingestion
""",
        
        "include/strategy/analyzer.hpp": """#pragma once

#include <vector>
#include <string>
#include "common/types.hpp"
#include "data_ingestion/order_book.hpp"

namespace strategy {

class Analyzer {
public:
    Analyzer(const nlohmann::json& config);
    ~Analyzer();
    
    // Analyze the order book and calculate imbalance metrics
    common::ImbalanceMetrics analyze(const data_ingestion::OrderBook& orderBook);
    
    // Calculate volume imbalance ratio
    double calculateBidAskRatio(const std::vector<common::PriceLevel>& bids,
                               const std::vector<common::PriceLevel>& asks) const;
    
    // Calculate price pressure
    double calculatePricePressure(const std::vector<common::PriceLevel>& bids,
                                 const std::vector<common::PriceLevel>& asks,
                                 double midPrice) const;
    
    // Calculate order flow acceleration
    double calculateAcceleration(const std::string& instrument) const;

private:
    // Configuration parameters
    size_t depth_;
    double weight_decay_;
    
    // Historical metrics for acceleration calculation
    std::unordered_map<std::string, std::vector<double>> historical_ratios_;
};

} // namespace strategy
"""
    }
    
    for path, content in header_files.items():
        create_file(os.path.join(base_dir, path), content)
    
    # Create sample implementation file
    impl_file = """#include "data_ingestion/order_book.hpp"
#include <algorithm>
#include <iostream>

namespace data_ingestion {

OrderBook::OrderBook() {}

OrderBook::~OrderBook() {}

void OrderBook::update(const std::string& instrument, 
                      const std::vector<common::PriceLevel>& bids,
                      const std::vector<common::PriceLevel>& asks) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& book = books_[instrument];
    
    {
        std::lock_guard<std::mutex> book_lock(book.mutex);
        book.bids = bids;
        book.asks = asks;
    }
}

void OrderBook::processUpdates() {
    // Process any pending updates if needed
    // This method would handle async updates in a real system
}

std::vector<common::PriceLevel> OrderBook::getBids(const std::string& instrument, size_t depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (books_.find(instrument) == books_.end()) {
        return {};
    }
    
    const auto& book = books_.at(instrument);
    std::lock_guard<std::mutex> book_lock(book.mutex);
    
    std::vector<common::PriceLevel> result;
    result.reserve(std::min(depth, book.bids.size()));
    
    for (size_t i = 0; i < std::min(depth, book.bids.size()); ++i) {
        result.push_back(book.bids[i]);
    }
    
    return result;
}

std::vector<common::PriceLevel> OrderBook::getAsks(const std::string& instrument, size_t depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (books_.find(instrument) == books_.end()) {
        return {};
    }
    
    const auto& book = books_.at(instrument);
    std::lock_guard<std::mutex> book_lock(book.mutex);
    
    std::vector<common::PriceLevel> result;
    result.reserve(std::min(depth, book.asks.size()));
    
    for (size_t i = 0; i < std::min(depth, book.asks.size()); ++i) {
        result.push_back(book.asks[i]);
    }
    
    return result;
}

double OrderBook::getMidPrice(const std::string& instrument) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (books_.find(instrument) == books_.end()) {
        return 0.0;
    }
    
    const auto& book = books_.at(instrument);
    std::lock_guard<std::mutex> book_lock(book.mutex);
    
    if (book.bids.empty() || book.asks.empty()) {
        return 0.0;
    }
    
    return (book.bids[0].price + book.asks[0].price) / 2.0;
}

double OrderBook::getBidVolume(const std::string& instrument, size_t depth) const {
    auto bids = getBids(instrument, depth);
    double total = 0.0;
    
    for (const auto& level : bids) {
        total += level.quantity;
    }
    
    return total;
}

double OrderBook::getAskVolume(const std::string& instrument, size_t depth) const {
    auto asks = getAsks(instrument, depth);
    double total = 0.0;
    
    for (const auto& level : asks) {
        total += level.quantity;
    }
    
    return total;
}

} // namespace data_ingestion
"""
    create_file(os.path.join(base_dir, "src/data_ingestion/order_book.cpp"), impl_file)
    
    # Create setup script
    setup_script = """#!/bin/bash
# Setup environment for the Order Imbalance System

# Update and install dependencies
echo "Installing system dependencies..."
sudo apt-get update
sudo apt-get install -y build-essential cmake libzmq3-dev libboost-all-dev git \
    python3 python3-pip python3-venv libssl-dev

# Create Python virtual environment
echo "Setting up Python virtual environment..."
python3 -m venv venv
source venv/bin/activate

# Install Python dependencies
echo "Installing Python dependencies..."
pip install -r python/requirements.txt

# Create build directory
mkdir -p build
cd build

# Configure project with CMake
echo "Configuring project with CMake..."
cmake ..

echo "Environment setup complete!"
echo "To build the project, run: cd build && make"
"""
    create_file(os.path.join(base_dir, "scripts/setup_env.sh"), setup_script)
    os.chmod(os.path.join(base_dir, "scripts/setup_env.sh"), 0o755)
    
    # Create Python requirements.txt
    requirements = """numpy==1.22.3
pandas==1.4.2
matplotlib==3.5.1
plotly==5.7.0
dash==2.3.1
scikit-learn==1.0.2
pytest==7.1.1
pyzmq==22.3.0
influxdb-client==1.27.0
psycopg2-binary==2.9.3
"""
    create_file(os.path.join(base_dir, "python/requirements.txt"), requirements)
    
    # Create empty __init__.py files for Python packages
    python_dirs = [
        "python/backtest",
        "python/research",
        "python/visualization"
    ]
    
    for directory in python_dirs:
        create_file(os.path.join(base_dir, directory, "__init__.py"), "")
    
    print(f"\nProject structure created in '{base_dir}' directory.")

if __name__ == "__main__":
    main()
