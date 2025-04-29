# Define an interface library for the common module (header-only)
add_library(common_module INTERFACE)

# Specify the include directory for this module
target_include_directories(common_module INTERFACE ${CMAKE_SOURCE_DIR}/include)

# Optionally, if you need to propagate any compile definitions or options,
# you can do it here (e.g., adding special flags for this module)
# target_compile_definitions(common_module INTERFACE SOME_DEFINITION)
