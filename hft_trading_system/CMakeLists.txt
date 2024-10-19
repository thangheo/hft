cmake_minimum_required(VERSION 3.10)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add third-party libraries
add_subdirectory(third_party/spdlog)
add_subdirectory(src)


# Add tests subdirectory
add_subdirectory(tests)
