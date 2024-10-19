
project(hft_trading_system)

# Add subdirectories for the modules
add_subdirectory(analytic_module)
add_subdirectory(api)
add_subdirectory(common)
#add_subdirectory(connectivity)
add_subdirectory(data_storage)
add_subdirectory(market_data)
add_subdirectory(oms)
add_subdirectory(risk_management)
add_subdirectory(strategy)


# Find required packages
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(hiredis REQUIRED)
find_package(redis++ REQUIRED)
find_package(nlohmann_json REQUIRED)

# Optional: Display important variables to ensure correct setup
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "Protobuf libraries: ${PROTOBUF_LIBRARIES}")
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")





# Add the main executable
add_executable(hft_trading_system main.cpp)



# Link all the modules to the main executable
target_link_libraries(hft_trading_system
    PRIVATE
    analytic_module
    api_module
    market_data_module
    oms_module
    data_storage_module
    market_data_module
    common_module
    # Add other modules as necessary
    spdlog::spdlog
    ${Boost_LIBRARIES}
    Threads::Threads
    ${PROTOBUF_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    hiredis
    redis++
    nlohmann_json::nlohmann_json
    pqxx
)

# Include directories for the main executable
target_include_directories(hft_trading_system PRIVATE ${CMAKE_SOURCE_DIR}/include)
