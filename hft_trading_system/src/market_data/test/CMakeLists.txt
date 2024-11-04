# Collect test sources
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Add executable for tests
add_executable(MyTradingSystemTests ${TEST_SOURCES})

# Link with main library and Google Test libraries
target_link_libraries(MyTradingSystemTests PRIVATE                 
                    order_execution_module 
                    order_tracking_module
                    market_data_module      # Market data handling module
                    spdlog::spdlog          # Logging library
                    ${Boost_LIBRARIES}      # Boost libraries
                    Threads::Threads        # Multi-threading support
                    ${PROTOBUF_LIBRARIES}   # Protocol Buffers library
                    OpenSSL::SSL            # OpenSSL SSL component
                    OpenSSL::Crypto         # OpenSSL Crypto component
                    hiredis                 # Redis client library
                    redis++                 # C++ wrapper for Redis client
                    nlohmann_json::nlohmann_json  # JSON serialization library
                    pqxx                    # C++ PostgreSQL client library
                    common_module           #common headers
                    real_time_data_module
                    serialization_module
                    risk_management_module
                    data_storage_module
                    gtest gmock gtest_main)

# Include directories for tests
target_include_directories(MyTradingSystemTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/googletest/include
)

# Add test to CMake's testing suite
add_test(NAME MyTradingSystemTests COMMAND MyTradingSystemTests)
