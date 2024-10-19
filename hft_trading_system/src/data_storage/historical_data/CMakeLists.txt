# Create a library for the historical_data module
add_library(historical_data_module historical_db.cpp)

# Include directories for this module
target_include_directories(historical_data_module PRIVATE
                                                 ${CMAKE_SOURCE_DIR}/include
                                                 ${CMAKE_SOURCE_DIR})
target_link_libraries(historical_data_module PRIVATE common_module)