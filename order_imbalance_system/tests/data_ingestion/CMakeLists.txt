# CMakeLists.txt for WebSocket tests

# Find GTest and GMock
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# WebSocket unit tests
set(WEBSOCKET_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/data_ingestion/websocket/test_websocket_client_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data_ingestion/websocket/test_connection_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data_ingestion/websocket/exchanges/test_binance_websocket_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data_ingestion/test_exchange_manager.cpp
)

# WebSocket integration tests
set(WEBSOCKET_INTEGRATION_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/data_ingestion/websocket/mock_exchange_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data_ingestion/websocket/test_websocket_integration.cpp
)

# Add the WebSocket unit tests executable
add_executable(websocket_unit_tests ${WEBSOCKET_TEST_SOURCES})
target_link_libraries(websocket_unit_tests
    PRIVATE
        data_ingestion_lib
        GTest::GTest
        GTest::Main
        GMock::GMock
        Boost::system
        Boost::thread
        OpenSSL::SSL
)

# Add the WebSocket integration tests executable
add_executable(websocket_integration_tests ${WEBSOCKET_INTEGRATION_TEST_SOURCES})
target_link_libraries(websocket_integration_tests
    PRIVATE
        data_ingestion_lib
        GTest::GTest
        GTest::Main
        GMock::GMock
        Boost::system
        Boost::thread
        OpenSSL::SSL
        websocketpp
)

# Add include directories 
target_include_directories(websocket_unit_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(websocket_integration_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Register the tests with CTest
include(GoogleTest)
gtest_discover_tests(websocket_unit_tests)
gtest_discover_tests(websocket_integration_tests)

# Install targets
install(TARGETS
    websocket_unit_tests
    websocket_integration_tests
    RUNTIME DESTINATION bin/tests
)