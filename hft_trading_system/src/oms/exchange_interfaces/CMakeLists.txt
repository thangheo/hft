# Create a library for the exchange interfaces module
add_library(exchange_interfaces_module binance_interface.cpp
                                        coinbase_interface.cpp)

# Include directories for this module
target_include_directories(exchange_interfaces_module PUBLIC ${CMAKE_SOURCE_DIR}/include)



find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(hiredis REQUIRED)
find_package(redis++ REQUIRED)
find_package(nlohmann_json REQUIRED)

# Link the submodules to the OMS library
target_link_libraries(exchange_interfaces_module PRIVATE  
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
)
