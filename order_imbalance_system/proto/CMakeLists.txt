set(PROTO_FILES
    market_data.proto
    order_book.proto
    execution_service.proto
)

# Check if we have the required executables for Protobuf and gRPC
if(NOT PROTOBUF_PROTOC_EXECUTABLE)
    # Try to find it in standard locations
    find_program(PROTOBUF_PROTOC_EXECUTABLE protoc)
    if(NOT PROTOBUF_PROTOC_EXECUTABLE)
        message(FATAL_ERROR "protoc executable not found. Please install Protocol Buffers or specify PROTOBUF_PROTOC_EXECUTABLE.")
    endif()
endif()

if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
    # Try to find it in standard locations
    find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
    if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
        message(FATAL_ERROR "grpc_cpp_plugin executable not found. Please install gRPC or specify GRPC_CPP_PLUGIN_EXECUTABLE.")
    endif()
endif()

# Generate protobuf and gRPC C++ files
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(proto ${PROTO_FILES})
    get_filename_component(proto_name ${proto} NAME_WE)
    
    # Protobuf outputs
    list(APPEND PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
    list(APPEND PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
    
    # gRPC outputs
    list(APPEND GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc")
    list(APPEND GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h")
    
    # Generate C++ files
    add_custom_command(
        OUTPUT
            "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc"
            "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h"
            "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc"
            "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h"
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS 
            --grpc_out="${CMAKE_CURRENT_BINARY_DIR}"
            --cpp_out="${CMAKE_CURRENT_BINARY_DIR}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}"
            --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${proto}"
        DEPENDS ${proto}
        COMMENT "Running protoc on ${proto}"
        VERBATIM
    )
endforeach()

# Create a library from the generated files
add_library(proto_lib ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})
target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf gRPC::grpc++)
target_include_directories(proto_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Make sure other targets can find the generated headers
set_target_properties(proto_lib PROPERTIES
    PUBLIC_HEADER "${PROTO_HDRS};${GRPC_HDRS}"
)