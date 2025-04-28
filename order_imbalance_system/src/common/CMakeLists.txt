# Common library
add_library(common
    zeromq_client.cpp
)

target_link_libraries(common
    PUBLIC
    cppzmq
    Boost::system
    Boost::thread
)

target_include_directories(common
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
