# Add library for the Market Data module
add_library(market_data_module    market_data_handler.cpp
)

# Add subdirectories for adapters and protocols
add_subdirectory(adapters)
add_subdirectory(protocols)
add_subdirectory(test)
find_package(prometheus-cpp CONFIG REQUIRED)

# Link necessary libraries to the Market Data module
target_link_libraries(market_data_module
    PRIVATE
    spdlog::spdlog        # Logging library
    ${Boost_LIBRARIES}    # Boost libraries
    hiredis               # Redis client library
    redis++               # C++ wrapper for Redis
    adapters_module
    protocols_module
    prometheus-cpp::core prometheus-cpp::pull prometheus-cpp::push)
  

# Include directories for the Market Data module
target_include_directories(market_data_module
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)
