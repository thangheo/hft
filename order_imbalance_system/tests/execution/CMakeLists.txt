cmake_minimum_required(VERSION 3.10)
project(OrderImbalanceTrading VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
Define test executables
set(TEST_EXECUTABLES
    test_twap_algorithm
    test_vwap_algorithm
    test_iceberg_algorithm
    test_execution_algorithm_factory
)

# Build each test executable
foreach(test_exec ${TEST_EXECUTABLES})
    add_executable(${test_exec} ${test_exec}.cpp)
    target_link_libraries(${test_exec}
        PRIVATE
        execution
        GTest::GTest
        GTest::Main
        GMock::GMock
        Threads::Threads
        UUID::UUID
    )
    target_include_directories(${test_exec}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    add_test(NAME ${test_exec} COMMAND ${test_exec})
endforeach()
# Find dependencies
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(uuid REQUIRED)
# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add test executables
add_executable(test_authentication_service test/test_authentication_service.cpp src/execution/authentication_service.cpp)
add_executable(test_rate_limit_controller test/test_rate_limit_controller.cpp src/execution/rate_limit_controller.cpp)
add_executable(test_request_prioritizer test/test_request_prioritizer.cpp src/execution/request_prioritizer.cpp)

# Link against Google Test and OpenSSL for authentication service
target_link_libraries(test_authentication_service ${GTEST_LIBRARIES} pthread ssl crypto)
target_link_libraries(test_rate_limit_controller ${GTEST_LIBRARIES} pthread)
target_link_libraries(test_request_prioritizer ${GTEST_LIBRARIES} pthread)

# Add tests to CTest
add_test(NAME AuthenticationServiceTest COMMAND test_authentication_service)
add_test(NAME RateLimitControllerTest COMMAND test_rate_limit_controller)
add_test(NAME RequestPrioritizerTest COMMAND test_request_prioritizer)
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
# Find GTest package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add test executables
add_executable(test_order_generator test_order_generator.cpp)
add_executable(test_order_type_selector test_order_type_selector.cpp)
add_executable(test_smart_order_router test_smart_order_router.cpp)
add_executable(integration_test_order_pipeline integration_test_order_pipeline.cpp)

# Link dependencies
target_link_libraries(test_order_generator execution_lib ${GTEST_LIBRARIES} pthread)
target_link_libraries(test_order_type_selector execution_lib ${GTEST_LIBRARIES} pthread)
target_link_libraries(test_smart_order_router execution_lib ${GTEST_LIBRARIES} pthread)
target_link_libraries(integration_test_order_pipeline execution_lib ${GTEST_LIBRARIES} pthread)

# Add tests to test runner
add_test(NAME OrderGeneratorTest COMMAND test_order_generator)
add_test(NAME OrderTypeSelectorTest COMMAND test_order_type_selector)
add_test(NAME SmartOrderRouterTest COMMAND test_smart_order_router)
add_test(NAME OrderPipelineIntegrationTest COMMAND integration_test_order_pipeline)