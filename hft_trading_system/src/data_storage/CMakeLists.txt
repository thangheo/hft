
add_library(data_storage_module storage_manager.cpp )

# Add subdirectories for the submodules
add_subdirectory(historical_data)
add_subdirectory(real_time_data)
add_subdirectory(serialization)

target_link_libraries(data_storage_module
    PRIVATE
    analytic_module
    api_module
    market_data_module
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

