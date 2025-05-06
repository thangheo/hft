cmake_minimum_required(VERSION 3.10)
project(OrderImbalanceTrading VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(uuid REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# Add test executables
add_executable(test_order test_order.cpp ${CMAKE_SOURCE_DIR}/src/execution/models/order.cpp)
add_executable(test_execution_report test_execution_report.cpp ${CMAKE_SOURCE_DIR}/src/execution/models/execution_report.cpp)
add_executable(test_exchange_api_client test_exchange_api_client.cpp ${CMAKE_SOURCE_DIR}/src/execution/exchange_api_client.cpp)

# Link libraries
target_link_libraries(test_order
    ${GTEST_LIBRARIES}
    Threads::Threads
    uuid
)

target_link_libraries(test_execution_report
    ${GTEST_LIBRARIES}
    Threads::Threads
    ${CMAKE_SOURCE_DIR}/build/liborder.a
)

target_link_libraries(test_exchange_api_client
    ${GTEST_LIBRARIES}
    Threads::Threads
    ${CMAKE_SOURCE_DIR}/build/liborder.a
    ${CMAKE_SOURCE_DIR}/build/libexecution_report.a
)

# Add tests to CTest
enable_testing()
add_test(NAME OrderTest COMMAND test_order)
add_test(NAME ExecutionReportTest COMMAND test_execution_report)
add_test(NAME ExchangeApiClientTest COMMAND test_exchange_api_client)

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_order test_execution_report test_exchange_api_client
)