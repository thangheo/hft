# src/execution/CMakeLists.txt

# Collect all source files
file(GLOB_RECURSE EXECUTION_SOURCES
    "*.cpp"
    "algorithms/*.cpp"
    "exchanges/*.cpp"
    "grpc/*.cpp"
    "models/*.cpp"
)

# WIP draft: enum names don't match trading_messages.proto yet; excluded until finished
list(FILTER EXECUTION_SOURCES EXCLUDE REGEX ".*/ExecutionDecisionEngine\\.cpp$")

# Define library
add_library(execution ${EXECUTION_SOURCES})

# Include directories
target_include_directories(execution
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include/execution
    ${CMAKE_SOURCE_DIR}/proto/generated  # Add proto generated directory
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(execution
    PUBLIC
    common
    PRIVATE
    proto_lib  # Fixed typo: proto_libs -> proto_lib
    ${Protobuf_LIBRARIES}
    ${gRPC_LIBRARIES}
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    curl
    jsoncpp
    uuid
)