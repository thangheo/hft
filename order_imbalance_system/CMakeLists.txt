cmake_minimum_required(VERSION 3.12)
project(OrderImbalanceTrading VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Add this near the top of CMakeLists.txt
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")
# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/external
    # other includes...
)
# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Dependencies
find_package(cppzmq REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# Include directories
include_directories(include)

# Add subdirectories
add_subdirectory(src/common)
# Uncomment as components are implemented
add_subdirectory(src/data_ingestion)
add_subdirectory(src/strategy)
add_subdirectory(src/execution)
add_subdirectory(src/risk)

# Add tests
enable_testing()
# add_subdirectory(tests)
# CMakeLists.txt (continued)
# Add common library
add_library(common STATIC
    src/common/logger.cpp
    src/common/config.cpp
    src/common/utils.cpp
)

# Add data ingestion library
add_library(data_ingestion STATIC
    src/data_ingestion/order_book.cpp
    src/data_ingestion/websocket_client.cpp
    src/data_ingestion/exchange_connector.cpp
)

# Link dependencies
target_link_libraries(data_ingestion
    common
    ${ZeroMQ_LIBRARIES}
    pthread
)

# Build main executable
add_executable(order_book_reconstructor
    src/data_ingestion/main.cpp
)

target_link_libraries(order_book_reconstructor
    data_ingestion
    common
    ${ZeroMQ_LIBRARIES}
    pthread
)

# Build tests
enable_testing()
add_executable(test_order_book
    tests/data_ingestion/test_order_book.cpp
)

target_link_libraries(test_order_book
    data_ingestion
    common
    ${ZeroMQ_LIBRARIES}
    pthread
)

add_test(NAME test_order_book COMMAND test_order_book)