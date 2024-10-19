# Add library for the API module
add_library(api_module data_api_server.cpp)

# Link necessary libraries
target_link_libraries(api_module
    PRIVATE
    spdlog::spdlog
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Include directories for this module
target_include_directories(api_module PRIVATE ${CMAKE_SOURCE_DIR}/include)
