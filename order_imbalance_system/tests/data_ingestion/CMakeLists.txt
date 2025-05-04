# CMakeLists.txt for WebSocket tests

# Find required packages
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# Include directories for tests
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# Source file definitions
# WebSocket unit tests
set(WEBSOCKET_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/test_connection_pool.cpp
)

# WebSocket integration tests
set(WEBSOCKET_INTEGRATION_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/mock_exchange_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/test_websocket_integration.cpp
)

# Orderbook sources - using the implementation files from src directory
set(ORDERBOOK_IMPL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/data_ingestion/orderbook/order_book.cpp
    ${CMAKE_SOURCE_DIR}/src/data_ingestion/orderbook/reconstructor.cpp
)

# Define all test executables
add_executable(websocket_unit_tests ${WEBSOCKET_TEST_SOURCES})
add_executable(websocket_integration_tests ${WEBSOCKET_INTEGRATION_TEST_SOURCES})
add_executable(test_websocket_client_base ${CMAKE_CURRENT_SOURCE_DIR}/websocket/test_websocket_client_base.cpp)
add_executable(test_connection_pool ${CMAKE_CURRENT_SOURCE_DIR}/websocket/test_connection_pool.cpp)
add_executable(test_binance_websocket_client ${CMAKE_CURRENT_SOURCE_DIR}/websocket/exchanges/test_binance_websocket_client.cpp)
add_executable(test_exchange_manager ${CMAKE_CURRENT_SOURCE_DIR}/test_exchange_manager.cpp)
add_executable(test_order_book ${CMAKE_CURRENT_SOURCE_DIR}/test_order_book.cpp ${CMAKE_SOURCE_DIR}/src/data_ingestion/orderbook/order_book.cpp)
# add_executable(test_order_book_reconstructor ${CMAKE_CURRENT_SOURCE_DIR}/test_order_book_reconstructor.cpp ${CMAKE_SOURCE_DIR}/src/data_ingestion/orderbook/reconstructor.cpp)

# Common libraries for most tests
set(COMMON_LIBRARIES
    data_ingestion
    GTest::GTest
    GTest::Main
    GMock::GMock
    Boost::system
    Boost::thread
    OpenSSL::SSL
)

# Link libraries for test executables
target_link_libraries(websocket_unit_tests PRIVATE ${COMMON_LIBRARIES})
target_link_libraries(websocket_integration_tests PRIVATE ${COMMON_LIBRARIES})
target_link_libraries(test_websocket_client_base PRIVATE ${COMMON_LIBRARIES})
target_link_libraries(test_connection_pool PRIVATE ${COMMON_LIBRARIES})
target_link_libraries(test_binance_websocket_client PRIVATE ${COMMON_LIBRARIES})
target_link_libraries(test_exchange_manager PRIVATE ${COMMON_LIBRARIES})

# Order book tests might not need all the same dependencies
target_link_libraries(test_order_book
    PRIVATE
        data_ingestion
        GTest::GTest
        GTest::Main
        GMock::GMock
)

# target_link_libraries(test_order_book_reconstructor
#     PRIVATE
#         data_ingestion
#         GTest::GTest
#         GTest::Main
#         GMock::GMock
#         Boost::system
# )

# Add include directories for all targets
foreach(target 
    websocket_unit_tests
    websocket_integration_tests
    test_websocket_client_base
    test_connection_pool
    test_binance_websocket_client
    test_exchange_manager
    test_order_book
    # test_order_book_reconstructor
)
    target_include_directories(${target}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

# Register all tests with CTest
include(GoogleTest)
gtest_discover_tests(websocket_unit_tests)
gtest_discover_tests(websocket_integration_tests)
gtest_discover_tests(test_websocket_client_base)
gtest_discover_tests(test_connection_pool)
gtest_discover_tests(test_binance_websocket_client)
gtest_discover_tests(test_exchange_manager)
gtest_discover_tests(test_order_book)
# gtest_discover_tests(test_order_book_reconstructor)

# Install targets
install(TARGETS
    websocket_unit_tests
    websocket_integration_tests
    test_websocket_client_base
    test_connection_pool
    test_binance_websocket_client
    test_exchange_manager
    test_order_book
    # test_order_book_reconstructor
    RUNTIME DESTINATION bin/tests
)