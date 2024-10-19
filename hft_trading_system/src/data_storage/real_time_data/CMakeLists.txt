# Create a library for the real_time_data module
add_library(real_time_data_module real_time_db.cpp)

# Include directories for this module
target_include_directories(real_time_data_module 
                PRIVATE 
                ${CMAKE_SOURCE_DIR}/include 
                ${CMAKE_SOURCE_DIR}
)

target_link_libraries(real_time_data_module PRIVATE common_module)