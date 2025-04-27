cmake_minimum_required(VERSION 3.15)
project(OrderImbalanceSystem VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add our custom module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Options
option(BUILD_TESTS "Build the tests" ON)
option(USE_SANITIZERS "Use address and undefined behavior sanitizers" OFF)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Find packages
find_package(Threads REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(Boost REQUIRED COMPONENTS system program_options)

# No need to include FlatBuffers here since it's not installed yet
# We'll add Protocol Buffers support later

# Set include directories from dependencies
include_directories(${ZeroMQ_INCLUDE_DIRS})

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
  if(USE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
  endif()
endif()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(libs)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()