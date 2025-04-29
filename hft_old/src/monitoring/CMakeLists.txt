# Create a library target for the Data Storage module
add_library(monitoring_module monitoring.cpp )

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(hiredis REQUIRED)
find_package(redis++ REQUIRED)
find_package(nlohmann_json REQUIRED)

# Use pkg-config to find libpqxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Include subdirectories for submodules
add_subdirectory(alert)
add_subdirectory(logging)
add_subdirectory(metric)

# Link necessary libraries and other modules to the Data Storage module
target_link_libraries(monitoring_module
    PRIVATE
    analytic_module         # Module for analytics
    ${PQXX_LIBRARIES}       # Correct linking of libpqxx
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
    common_module           #common headers
    alert_module
    real_time_data_module
    serialization_module
)
