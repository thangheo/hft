# Specify the minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(hft_trading_system)

# Manually specify NLopt paths
set(NLOPT_INCLUDE_DIR "/usr/local/include")
set(NLOPT_LIBRARY "/usr/local/lib/libnlopt.so")

# Include NLopt headers
include_directories(${NLOPT_INCLUDE_DIR})

# Create the executable
add_executable(hft_trading_system ../hawkes_process.cpp hawkes_test.cpp)

# Link the NLopt library to the executable
target_link_libraries(hft_trading_system ${NLOPT_LIBRARY})
