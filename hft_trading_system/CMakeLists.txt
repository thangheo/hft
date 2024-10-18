cmake_minimum_required(VERSION 3.10)
project(hft_trading_system)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    include
    /usr/local/include
)

# Recursively include all directories in src/
include_directories(${CMAKE_SOURCE_DIR})

# Find all source files recursively in the 'src' directory
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add spdlog as a subdirectory (since it is part of third_party)
add_subdirectory(third_party/spdlog)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(hiredis REQUIRED)
find_package(redis++ REQUIRED)
find_package(nlohmann_json REQUIRED)
# Add executable
add_executable(hft_trading_system ${SOURCES})

# Link libraries to the executable
target_link_libraries(hft_trading_system
    PRIVATE
    Boost::system
    Boost::filesystem
    Boost::thread
    Threads::Threads
    ${PROTOBUF_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
    hiredis
    redis++
    nlohmann_json::nlohmann_json)

# Additional settings for build configuration
target_include_directories(hft_trading_system
    PRIVATE
    ${Protobuf_INCLUDE_DIRS}
)

# Optional: Display important variables to ensure correct setup
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "Protobuf libraries: ${PROTOBUF_LIBRARIES}")
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
