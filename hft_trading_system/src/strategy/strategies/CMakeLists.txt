# Create the trategies_module
add_library(trategies_module mean_reversion_strategy.cpp
                            momentum_strategy.cpp)


# Include directories 
target_include_directories(trategies_module PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link the submodules 
target_link_libraries(trategies_module PRIVATE common_module )
