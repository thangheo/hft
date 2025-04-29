# Set the C++ standard to C++17 (required by libpqxx 7.x)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Create a library for the historical_data module
add_library(historical_data_module historical_db.cpp)

# Include directories for this module
target_include_directories(historical_data_module PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${PQXX_INCLUDE_DIRS}  # Ensure libpqxx include directories are added
)

# Find required packages
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(hiredis REQUIRED)
find_package(redis++ REQUIRED)
find_package(nlohmann_json REQUIRED)

# Use pkg-config to find libpqxx
find_package(PkgConfig REQUIRED)
#pkg_check_modules(PQXX REQUIRED libpqxx)

# Link necessary libraries and other modules to the Data Storage module
find_package(PostgreSQL REQUIRED)

# Link necessary libraries and other modules
target_link_libraries(historical_data_module
    PRIVATE
    analytic_module
    market_data_module
    spdlog::spdlog
    ${Boost_LIBRARIES}
    Threads::Threads
    ${PROTOBUF_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    hiredis
    redis++
    nlohmann_json::nlohmann_json
    ${PQXX_LIBRARIES}
    ${PostgreSQL_LIBRARIES}  # Add libpq here
    common_module
    real_time_data_module
    serialization_module
)

# If libpqxx is installed in a custom directory, add its path:
# link_directories(/path/to/libpqxx/lib)
