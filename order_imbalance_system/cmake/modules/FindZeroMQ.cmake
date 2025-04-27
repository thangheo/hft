# FindZeroMQ.cmake - Find ZeroMQ library
# 
# This module defines the following variables:
#   ZeroMQ_FOUND - System has ZeroMQ
#   ZeroMQ_INCLUDE_DIRS - ZeroMQ include directories
#   ZeroMQ_LIBRARIES - Libraries needed to use ZeroMQ
#
# This module will use pkg-config if found to help find the library

# Try to find ZeroMQ using pkg-config
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ZeroMQ QUIET libzmq)
endif()

# Find the include directories
find_path(ZeroMQ_INCLUDE_DIR
  NAMES zmq.h
  PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
  PATH_SUFFIXES include
)

# Find the libraries
find_library(ZeroMQ_LIBRARY
  NAMES zmq libzmq
  PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64
)

# Set variables for standard Find module
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ
  REQUIRED_VARS ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR
)

if(ZeroMQ_FOUND)
  set(ZeroMQ_LIBRARIES ${ZeroMQ_LIBRARY})
  set(ZeroMQ_INCLUDE_DIRS ${ZeroMQ_INCLUDE_DIR})
endif()

# Hide these variables in GUI tools
mark_as_advanced(ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY)