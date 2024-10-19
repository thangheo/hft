# Create a library for the serialization module
add_library(serialization_module serializer.cpp market_data.pb.cc order_data.pb.cc)

# Link Protobuf library
find_package(Protobuf REQUIRED)
target_link_libraries(serialization_module PRIVATE ${Protobuf_LIBRARIES})

# Include directories for this module
target_include_directories(serialization_module PUBLIC ${CMAKE_SOURCE_DIR}/include ${Protobuf_INCLUDE_DIRS})
