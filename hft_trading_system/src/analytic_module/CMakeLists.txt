# Add library for the analytic module
add_library(analytic_module data_exporter.cpp)

# Link necessary libraries
target_link_libraries(analytic_module
    PRIVATE
    spdlog::spdlog
    ${Boost_LIBRARIES}
)

# Include directories for this module
target_include_directories(analytic_module PRIVATE ${CMAKE_SOURCE_DIR}/include)
