# Create the backtesting_module library
add_library(backtesting_module backtester.cpp)

# Include directories for backtesting_module module
target_include_directories(backtesting_module PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link the submodules to the backtesting_module library
target_link_libraries(backtesting_module PRIVATE common_module )
