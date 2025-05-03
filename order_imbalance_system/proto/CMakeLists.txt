set(PROTO_FILES
    market_data.proto
    order_book.proto
    execution_service.proto
)

# Set explicit paths for required executables
if(NOT PROTOBUF_PROTOC_EXECUTABLE)
    find_program(PROTOBUF_PROTOC_EXECUTABLE protoc REQUIRED)
    message(STATUS "Found protoc: ${PROTOBUF_PROTOC_EXECUTABLE}")
endif()

if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
    # First, try using the system path without quotes
    set(GRPC_CPP_PLUGIN_EXECUTABLE /usr/bin/grpc_cpp_plugin)
    
    if(NOT EXISTS ${GRPC_CPP_PLUGIN_EXECUTABLE})
        # Fall back to find_program if explicit path doesn't work
        find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)
    endif()
    
    message(STATUS "Using gRPC plugin: ${GRPC_CPP_PLUGIN_EXECUTABLE}")
    
    # Verify the plugin is executable
    # execute_process(
    #     COMMAND ${GRPC_CPP_PLUGIN_EXECUTABLE} --help
    #     RESULT_VARIABLE GRPC_PLUGIN_TEST
    #     ERROR_QUIET
    #     OUTPUT_QUIET
    # )
    
    # if(NOT GRPC_PLUGIN_TEST EQUAL 0)
    #     message(FATAL_ERROR "gRPC plugin at ${GRPC_CPP_PLUGIN_EXECUTABLE} exists but is not executable or is invalid")
    # endif()
endif()

# Generate protobuf and gRPC C++ files
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

# Create output directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

foreach(proto ${PROTO_FILES})
    get_filename_component(proto_name ${proto} NAME_WE)
    
    # Define output files
    set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
    set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
    set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc")
    set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h")
    
    # Add to lists
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    list(APPEND GRPC_SRCS ${GRPC_SRC})
    list(APPEND GRPC_HDRS ${GRPC_HDR})
    
    # First generate the protobuf files without gRPC
    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
             -I ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/${proto}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${proto}
        COMMENT "Generating protobuf files for ${proto}"
        VERBATIM
    )
    add_custom_command(
    OUTPUT ${GRPC_SRC} ${GRPC_HDR}
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    # ARGS --grpc_out=generate_mock_code=true:${CMAKE_CURRENT_BINARY_DIR}
    ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/${proto}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${proto} ${PROTO_SRC} ${PROTO_HDR}
    COMMENT "Generating gRPC files for ${proto}"
    VERBATIM
)
endforeach()

# Create a library from the generated files
add_library(proto_lib ${PROTO_SRCS} ${GRPC_SRCS} ${PROTO_HDRS} ${GRPC_HDRS})
target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf gRPC::grpc++)
target_include_directories(proto_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# target_compile_definitions(proto_lib PUBLIC GRPC_USE_PROTO_LITE)
target_compile_definitions(proto_lib PUBLIC 
    GRPC_USE_PROTO_LITE
    GOOGLE_PROTOBUF_NO_RTTI
    GPR_FORBID_UNREACHABLE_CODE
)
# Make sure other targets can find the generated headers
set_target_properties(proto_lib PROPERTIES
    PUBLIC_HEADER "${PROTO_HDRS};${GRPC_HDRS}"
)