cmake_minimum_required(VERSION 3.10)
project(hft_trading_system)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(include)
include_directories(/usr/local/include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Libraries
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

# Add executable
add_executable(hft_trading_system ${SOURCES})

# Link libraries
target_link_libraries(hft_trading_system
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${PROTOBUF_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
)
