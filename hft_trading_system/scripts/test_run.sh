#!/bin/bash

# test_run.sh - Script to run unit tests

# Exit immediately if a command exits with a non-zero status
set -e

echo "Running unit tests..."

# Navigate to the build directory
cd build

# Run tests
ctest --output-on-failure

echo "Unit tests completed."
