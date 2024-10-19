# Add library for the market data module
add_library(market_data_module market_data_handler.cpp)
add_subdirectory(adapters)
add_subdirectory(protocols)
# Link necessary libraries
target_link_libraries(market_data_module
    PRIVATE
    spdlog::spdlog
    ${Boost_LIBRARIES}
    hiredis
    redis++
)

# Include directories for this module
target_include_directories(market_data_module PRIVATE ${CMAKE_SOURCE_DIR}/include)
