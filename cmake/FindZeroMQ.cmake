################################################################################
# Copyright (C) 2012-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################
#
# Authors:
#
#   Mohammad Al-Turany
#   Dario Berzano
#   Dennis Klein
#   Matthias Richter
#   Alexey Rybalchenko
#   Florian Uhlig
#
#
# #############################
# # Locate the ZeroMQ library #
# #############################
#
#
# Usage:
#
#   find_package(ZeroMQ [version] [QUIET] [REQUIRED])
#
#
# Defines the following variables:
#
#   ZeroMQ_FOUND - Found the ZeroMQ library
#   ZeroMQ_INCLUDE_DIR (CMake cache) - Include directory
#   ZeroMQ_LIBRARY_SHARED (CMake cache) - Path to shared libzmq
#   ZeroMQ_LIBRARY_STATIC (CMake cache) - Path to static libzmq
#   ZeroMQ_VERSION - full version string
#   ZeroMQ_VERSION_MAJOR - major version component
#   ZeroMQ_VERSION_MINOR - minor version component
#   ZeroMQ_VERSION_PATCH - patch version component
#
#
# Accepts the following variables as hints for installation directories:
#
#   ZEROMQ_ROOT (CMake var, ENV var)
#
#
# If the above variables are not defined, or if ZeroMQ could not be found there,
# it will look for it in the system directories. Custom ZeroMQ installations
# will always have priority over system ones.
#

if(NOT ZEROMQ_ROOT)
  set(ZEROMQ_ROOT $ENV{ZEROMQ_ROOT})
endif()

find_path(ZeroMQ_INCLUDE_DIR
  NAMES zmq.h zmq_utils.h
  HINTS ${ZEROMQ_ROOT} $ENV{ZEROMQ_ROOT}
  PATH_SUFFIXES include
  DOC "ZeroMQ include directories"
)

find_library(ZeroMQ_LIBRARY_SHARED
  NAMES libzmq.dylib libzmq.so
  HINTS ${ZEROMQ_ROOT} $ENV{ZEROMQ_ROOT}
  PATH_SUFFIXES lib
  DOC "Path to libzmq.dylib or libzmq.so"
)

find_library(ZeroMQ_LIBRARY_STATIC NAMES libzmq.a
  HINTS ${ZEROMQ_ROOT} $ENV{ZEROMQ_ROOT}
  PATH_SUFFIXES lib
  DOC "Path to libzmq.a"
)

find_file(ZeroMQ_HEADER_FILE "zmq.h"
  ${ZeroMQ_INCLUDE_DIR}
  NO_DEFAULT_PATH
)
if(DEFINED ZeroMQ_HEADER_FILE)
  file(READ "${ZeroMQ_HEADER_FILE}" _ZeroMQ_HEADER_FILE_CONTENT)
  string(REGEX MATCH "#define ZMQ_VERSION_MAJOR ([0-9])" _MATCH "${_ZeroMQ_HEADER_FILE_CONTENT}")
  set(ZeroMQ_VERSION_MAJOR ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define ZMQ_VERSION_MINOR ([0-9])" _MATCH "${_ZeroMQ_HEADER_FILE_CONTENT}")
  set(ZeroMQ_VERSION_MINOR ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define ZMQ_VERSION_PATCH ([0-9])" _MATCH "${_ZeroMQ_HEADER_FILE_CONTENT}")
  set(ZeroMQ_VERSION_PATCH ${CMAKE_MATCH_1})
  set(ZeroMQ_VERSION "${ZeroMQ_VERSION_MAJOR}.${ZeroMQ_VERSION_MINOR}.${ZeroMQ_VERSION_PATCH}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ
    REQUIRED_VARS ZeroMQ_LIBRARY_SHARED ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY_STATIC
    VERSION_VAR ZeroMQ_VERSION
)

if(ZeroMQ_FOUND AND NOT TARGET libzmq)
  add_library(libzmq SHARED IMPORTED)
  set_target_properties(libzmq PROPERTIES
    IMPORTED_LOCATION ${ZeroMQ_LIBRARY_SHARED}
    INTERFACE_INCLUDE_DIRECTORIES ${ZeroMQ_INCLUDE_DIR}
    INTERFACE_LINK_LIBRARIES "$<$<PLATFORM_ID:Linux>:rt>;pthread"
  )
endif()

mark_as_advanced(
    ZeroMQ_LIBRARIES
    ZeroMQ_LIBRARY_SHARED
    ZeroMQ_LIBRARY_STATIC
    ZeroMQ_HEADER_FILE
    ZeroMQ_VERSION_MAJOR
    ZeroMQ_VERSION_MINOR
    ZeroMQ_VERSION_PATCH
)
