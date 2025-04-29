# Create a library for the real_time_data module
add_library(real_time_data_module real_time_db.cpp)

# Include directories for this module
target_include_directories(real_time_data_module 
                PRIVATE 
                ${CMAKE_SOURCE_DIR}/include 
                ${CMAKE_SOURCE_DIR}
)

target_link_libraries(real_time_data_module PRIVATE common_module 
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
                        serialization_module
                        pqxx
)
