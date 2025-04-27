# FindZeroMQ.cmake - Find ZeroMQ library
# This module defines
#  ZeroMQ_INCLUDE_DIRS - where to find zmq.h
#  ZeroMQ_LIBRARIES - the libraries needed to use ZeroMQ
#  ZeroMQ_FOUND - If false, do not try to use ZeroMQ

find_path(ZeroMQ_INCLUDE_DIR zmq.h
    PATHS /usr/include /usr/local/include
)

find_library(ZeroMQ_LIBRARY NAMES zmq
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ DEFAULT_MSG ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR)

if(ZeroMQ_FOUND)
    set(ZeroMQ_LIBRARIES ${ZeroMQ_LIBRARY})
    set(ZeroMQ_INCLUDE_DIRS ${ZeroMQ_INCLUDE_DIR})
endif()

mark_as_advanced(ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR)
