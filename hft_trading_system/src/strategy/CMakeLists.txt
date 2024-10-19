# Create the strategy_module library
add_library(strategy_module strategy_engine.cpp)

# Add subdirectories for the submodules
add_subdirectory(backtesting)
add_subdirectory(strategies)

# Include directories for strategy_module module
target_include_directories(strategy_module PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link the submodules to the strategy_module library
target_link_libraries(strategy_module PRIVATE common_module trategies_module)
