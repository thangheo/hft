# Enable testing
enable_testing()

# Find GoogleTest
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# Define the test executables
set(RISK_TEST_SOURCES
    test_stop_loss_manager.cpp
    test_drawdown_monitor.cpp
    test_circuit_breaker.cpp
    test_balance_reconciliation.cpp
    test_post_trade_monitor.cpp
)

# Create test executable
add_executable(risk_posttrade_tests ${RISK_TEST_SOURCES})

# Link dependencies
target_link_libraries(risk_posttrade_tests
    PRIVATE
    risk_posttrade
    GTest::GTest
    GTest::Main
    GMock::GMock
)

# Include directories
target_include_directories(risk_posttrade_tests
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Add tests to CTest
gtest_discover_tests(risk_posttrade_tests)