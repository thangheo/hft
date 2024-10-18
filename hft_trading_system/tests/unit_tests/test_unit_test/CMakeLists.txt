cmake_minimum_required(VERSION 3.10)
project(UnitTests)

# Google Test setup
add_subdirectory(${CMAKE_SOURCE_DIR}/../../../third_party/googletest)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../../../src)
include_directories(${CMAKE_SOURCE_DIR})

# Add the test executable
add_executable(OrderBookTest test_orderbook.cpp)

# Link Google Test and necessary libraries
target_link_libraries(OrderBookTest gtest gtest_main)
