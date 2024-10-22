cmake_minimum_required(VERSION 3.10)
project(hft_trading_system)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(Protobuf_INCLUDE_DIR "/usr/local/include")  # Adjust this path to where your Protobuf headers are located
#set(Protobuf_LIBRARY "/usr/local/lib/libprotobuf.so")  # Adjust this path to the Protobuf library
# Set the protobuf_DIR to help find the correct configuration files
#set(protobuf_DIR "/usr/local/lib/cmake/protobuf")
find_package(Protobuf REQUIRED)
#find_package(absl REQUIRED)


# Find required packages
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(hiredis REQUIRED)
find_package(redis++ REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)


# Optional: Display important variables to ensure correct setup
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "Protobuf libraries: ${PROTOBUF_LIBRARIES}")
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

# Add third-party libraries
add_subdirectory(third_party/spdlog)  # Remove this if you're using spdlog via find_package()
#add_subdirectory(third_party/absl)
add_subdirectory(src)
add_subdirectory(configs)
# Add tests subdirectory
add_subdirectory(tests)

# Add the main executable
add_executable(hft_trading_system src/main.cpp)  # Moved main.cpp reference here

# Link all the modules and libraries to the main executable
target_link_libraries(hft_trading_system
    PRIVATE
    analytic_module
    strategy_module
    market_data_module
    oms_module
    risk_management_module
    data_storage_module
    common_module
    configs_module
    spdlog::spdlog
    ${Boost_LIBRARIES}
    Threads::Threads
    ${PROTOBUF_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    hiredis
    redis++
    nlohmann_json::nlohmann_json
    ${PQXX_LIBRARIES}  # Add this line to link libpqxx correctly
    prometheus-cpp::core 
    prometheus-cpp::pull 
    prometheus-cpp::push 
    )
# Include directories for the main executable
target_include_directories(hft_trading_system PRIVATE ${CMAKE_SOURCE_DIR}/include)
