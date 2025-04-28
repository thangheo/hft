# Find required packages
find_package(Protobuf REQUIRED)

# Get all proto files
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

# Generate C++ files from proto files
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Create library with generated files
add_library(protolib ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(protolib ${PROTOBUF_LIBRARIES})
target_include_directories(protolib PUBLIC ${PROTOBUF_INCLUDE_DIRS})