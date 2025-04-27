#!/bin/bash
# Setup environment for the Order Imbalance System with binary-only packages

# Exit on error
set -e

echo "Setting up Order Imbalance System environment..."

# Update and install dependencies
echo "Installing system dependencies..."
sudo apt-get update
sudo apt-get install -y build-essential cmake libzmq3-dev libboost-all-dev \
    git python3.12 python3-venv python3-full libssl-dev python3.12-dev

# Check if Python 3.12 is installed correctly
if ! command -v python3.12 &> /dev/null; then
    echo "Error: Python 3.12 not found. Please install it and try again."
    exit 1
fi

# Create Python virtual environment
echo "Setting up Python virtual environment..."
if [ -d "venv" ]; then
    echo "Virtual environment already exists. Removing and recreating..."
    rm -rf venv
fi

python3.12 -m venv venv

# Activate virtual environment
source venv/bin/activate

# Verify activation
if [[ "$VIRTUAL_ENV" == "" ]]; then
    echo "Error: Virtual environment activation failed."
    exit 1
fi

# Install base packages first
echo "Installing base Python packages..."
pip install --upgrade pip
pip install --upgrade setuptools wheel

# Force binary-only installation
echo "Installing packages with binary-only option..."

# Install latest numpy binary first (this is crucial)
pip install --only-binary=:all: numpy

# Install scipy binary
pip install --only-binary=:all: scipy

# Install pandas binary
pip install --only-binary=:all: pandas

# Install scikit-learn binary
pip install --only-binary=:all: scikit-learn

# Install matplotlib binary
pip install --only-binary=:all: matplotlib

# Install remaining packages with binary preference
pip install --prefer-binary plotly==5.7.0
pip install --prefer-binary dash==2.3.1
pip install --prefer-binary pytest==7.1.1
pip install --prefer-binary pyzmq>=26.0.0
pip install --prefer-binary influxdb-client==1.27.0
pip install --prefer-binary psycopg2-binary>=2.9.5

# Create necessary directory structure if it doesn't exist
echo "Creating project directory structure..."
mkdir -p src
mkdir -p libs
mkdir -p tests

# Create minimal CMakeLists.txt files in each directory
if [ ! -f src/CMakeLists.txt ]; then
    echo "# Source directory CMakeLists.txt" > src/CMakeLists.txt
    echo "# Add your source files here" >> src/CMakeLists.txt
fi

if [ ! -f libs/CMakeLists.txt ]; then
    echo "# Libraries directory CMakeLists.txt" > libs/CMakeLists.txt
    echo "# Add your libraries here" >> libs/CMakeLists.txt
fi

if [ ! -f tests/CMakeLists.txt ]; then
    echo "# Tests directory CMakeLists.txt" > tests/CMakeLists.txt
    echo "# Add your tests here" >> tests/CMakeLists.txt
fi

# Create build directory
mkdir -p build
cd build

# Create a FindZeroMQ.cmake file to help CMake find ZeroMQ
echo "Creating FindZeroMQ.cmake file..."
mkdir -p ../cmake
cat > ../cmake/FindZeroMQ.cmake << 'EOF'
# FindZeroMQ.cmake - Find ZeroMQ library
# This module defines
#  ZeroMQ_INCLUDE_DIRS - where to find zmq.h
#  ZeroMQ_LIBRARIES - the libraries needed to use ZeroMQ
#  ZeroMQ_FOUND - If false, do not try to use ZeroMQ

find_path(ZeroMQ_INCLUDE_DIR zmq.h
    PATHS /usr/include /usr/local/include
)

find_library(ZeroMQ_LIBRARY NAMES zmq
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ DEFAULT_MSG ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR)

if(ZeroMQ_FOUND)
    set(ZeroMQ_LIBRARIES ${ZeroMQ_LIBRARY})
    set(ZeroMQ_INCLUDE_DIRS ${ZeroMQ_INCLUDE_DIR})
endif()

mark_as_advanced(ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR)
EOF

# Configure project with CMake
echo "Configuring project with CMake..."
cmake -DCMAKE_MODULE_PATH=$(realpath ../cmake) .. || echo "CMake configuration failed. Please check error messages."

echo "Environment setup complete!"
echo "To build the project, run: cd build && make"
echo ""
echo "Installed package versions:"
pip list | grep -E "numpy|scipy|pandas|scikit-learn|matplotlib|pyzmq|psycopg2"