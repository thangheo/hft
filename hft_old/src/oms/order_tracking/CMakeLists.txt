# Create a library for the order tracking module
add_library(order_tracking_module order_tracker.cpp)

# Include directories for this module
target_include_directories(order_tracking_module PUBLIC ${CMAKE_SOURCE_DIR}/include)
