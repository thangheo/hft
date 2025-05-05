cmake_minimum_required(VERSION 3.14)
project(risk_portfolio_tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Include project headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add test source files
set(TEST_SOURCES
    test_main.cpp
    test_position_tracker.cpp
    test_portfolio_manager.cpp
    test_collateral_manager.cpp
    test_rebalancing_engine.cpp
    test_portfolio_integration.cpp
)

# Create test executable
add_executable(run_tests ${TEST_SOURCES})

# Link against Google Test and your project libraries
target_link_libraries(run_tests 
    ${GTEST_LIBRARIES}
    pthread
    risk_common
    risk_portfolio
)

# Add test
enable_testing()
add_test(NAME portfolio_tests COMMAND run_tests)