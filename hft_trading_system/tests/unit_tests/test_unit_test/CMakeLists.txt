# Add Google Test framework (assuming you have it in third_party)
enable_testing()

# Create an executable for unit tests
add_executable(unit_tests test_orderbook.cpp)

# Link the test executable with necessary modules and libraries
target_link_libraries(unit_tests
    PRIVATE
    analytic_module
    api_module
    market_data_module
    spdlog::spdlog
    gtest
    gtest_main
)

# Register the tests
add_test(NAME UnitTests COMMAND unit_tests)
