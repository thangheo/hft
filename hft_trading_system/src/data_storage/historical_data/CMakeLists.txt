# Create a library for the historical_data module
add_library(historical_data_module historical_db.cpp)

# Include directories for this module
target_include_directories(historical_data_module PUBLIC ${CMAKE_SOURCE_DIR}/include)
