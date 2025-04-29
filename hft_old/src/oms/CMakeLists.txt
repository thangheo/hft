# Create the OMS library
add_library(oms_module oms.cpp)

# Add subdirectories for the submodules
add_subdirectory(exchange_interfaces)
add_subdirectory(order_execution)
add_subdirectory(order_tracking)

# Include directories for OMS module
target_include_directories(oms_module PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(hiredis REQUIRED)
find_package(redis++ REQUIRED)
find_package(nlohmann_json REQUIRED)

# Link the submodules to the OMS library
target_link_libraries(oms_module PRIVATE exchange_interfaces_module 
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
