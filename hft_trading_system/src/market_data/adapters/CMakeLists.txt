# Create a library for the historical_data module
add_library(adapters_module binance_adapter.cpp)

# Include directories for this module
target_include_directories(adapters_module PRIVATE
                                                 ${CMAKE_SOURCE_DIR}/include
                                                 ${CMAKE_SOURCE_DIR})
target_link_libraries(adapters_module PRIVATE common_module)