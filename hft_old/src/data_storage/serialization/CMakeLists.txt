# Create a library for the serialization module
#add_library(serialization_module serializer.cpp market_data.pb.cc order_data.pb.cc)
#protobuf
#find_package(Protobuf REQUIRED)

# Link Protobuf library
#target_link_libraries(serialization_module PRIVATE ${Protobuf_LIBRARIES})

# Include directories for this module
#target_include_directories(serialization_module PUBLIC ${CMAKE_SOURCE_DIR}/include ${Protobuf_INCLUDE_DIRS})





find_package(Protobuf REQUIRED)

# Generate the Protocol Buffer source files from .proto files
set(PROTO_FILES
    #${CMAKE_SOURCE_DIR}/src/data_storage/serialization/market_data.proto
    ${CMAKE_SOURCE_DIR}/proto/market_data.proto
    ${CMAKE_SOURCE_DIR}/proto/order_data.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Include the generated files and link the library
add_library(serialization_module serializer.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

# Link Protocol Buffers library
target_link_libraries(serialization_module
    PRIVATE
    protobuf::libprotobuf
    market_data_module      # Market data handling module
    spdlog::spdlog          # Logging library
    ${Boost_LIBRARIES}      # Boost libraries
    Threads::Threads        # Multi-threading support
    ${PROTOBUF_LIBRARIES}   # Protocol Buffers library
    OpenSSL::SSL            # OpenSSL SSL component
    OpenSSL::Crypto         # OpenSSL Crypto component
    hiredis                 # Redis client library
    redis++                 # C++ wrapper for Redis client
    nlohmann_json::nlohmann_json  # JSON serialization library
    pqxx                    # C++ PostgreSQL client library
    common_module           #common headers
    real_time_data_module
    pqxx
 )
