# Create the OMS library
add_library(oms_module oms.cpp)

# Add subdirectories for the submodules
add_subdirectory(exchange_interfaces)
add_subdirectory(order_execution)
add_subdirectory(order_tracking)

# Include directories for OMS module
target_include_directories(oms_module PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link the submodules to the OMS library
target_link_libraries(oms_module PRIVATE exchange_interfaces_module order_execution_module order_tracking_module)
