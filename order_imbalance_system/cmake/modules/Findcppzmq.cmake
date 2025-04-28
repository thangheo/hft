# Find the ZeroMQ libraries
find_package(ZeroMQ QUIET)

# ZeroMQ libraries found
if(ZeroMQ_FOUND)
    find_path(CPPZMQ_INCLUDE_DIR
        NAMES zmq.hpp
        PATHS ${ZEROMQ_INCLUDE_DIR} ${CMAKE_INCLUDE_PATH}
    )

    if(CPPZMQ_INCLUDE_DIR)
        set(CPPZMQ_FOUND ON)
    endif()
endif()

# Output variables
if(CPPZMQ_FOUND)
    set(CPPZMQ_INCLUDE_DIRS ${CPPZMQ_INCLUDE_DIR})
    set(CPPZMQ_LIBRARIES ${ZeroMQ_LIBRARIES})
    if(NOT CPPZMQ_FIND_QUIETLY)
        message(STATUS "Found CPPZMQ: ${CPPZMQ_INCLUDE_DIRS}")
    endif()
else()
    set(CPPZMQ_INCLUDE_DIRS)
    set(CPPZMQ_LIBRARIES)
    if(CPPZMQ_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find CPPZMQ")
    endif()
endif()

mark_as_advanced(CPPZMQ_INCLUDE_DIR)
