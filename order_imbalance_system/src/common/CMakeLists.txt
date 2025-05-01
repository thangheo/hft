set(COMMON_SOURCES
    zeromq_client.cpp
    logger.cpp
)

add_library(common ${COMMON_SOURCES})

target_link_libraries(common
    PUBLIC
        ZeroMQ::ZeroMQ
        cppzmq
        Boost::system
        Boost::filesystem
        Threads::Threads
)

target_include_directories(common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Install headers
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/
    DESTINATION include/common
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install the library
install(TARGETS common
    EXPORT CommonTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)