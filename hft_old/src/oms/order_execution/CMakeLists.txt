# Create a library for the order execution module
add_library(order_execution_module order_executor.cpp)

# Include directories for this module
target_include_directories(order_execution_module PUBLIC ${CMAKE_SOURCE_DIR}/include)
