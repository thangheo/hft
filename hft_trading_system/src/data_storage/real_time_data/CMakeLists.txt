# Create a library for the real_time_data module
add_library(real_time_data_module real_time_db.cpp)

# Include directories for this module
target_include_directories(real_time_data_module PUBLIC ${CMAKE_SOURCE_DIR}/include)
