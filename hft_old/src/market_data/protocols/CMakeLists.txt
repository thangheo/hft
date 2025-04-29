add_library(protocols_module websocket_client.cpp)
# Link necessary libraries
target_link_libraries(protocols_module
    PRIVATE
    spdlog::spdlog
    ${Boost_LIBRARIES}
  
)

# Include directories for this module
target_include_directories(protocols_module PRIVATE ${CMAKE_SOURCE_DIR}/include)
