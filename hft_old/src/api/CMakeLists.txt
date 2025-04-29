# Set the project name
project(api_server)

# Locate necessary packages
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(spdlog REQUIRED)

# Create an executable target for the API module
add_executable(api_server data_api_server.cpp)

# Specify include directories for the API module
target_include_directories(api_server
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/data_storage/historical_data
    ${CMAKE_SOURCE_DIR}/src/data_storage/real_time_data
    
)

# Link the required libraries to the API module
target_link_libraries(api_server
    PRIVATE
    spdlog::spdlog        # Logging library
    ${Boost_LIBRARIES}    # Boost libraries
    OpenSSL::SSL          # OpenSSL SSL component
    OpenSSL::Crypto       # OpenSSL Crypto component
)
