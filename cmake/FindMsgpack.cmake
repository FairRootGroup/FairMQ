################################################################################
# Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

find_path(MSGPACK_INCLUDE_DIR
  NAMES msgpack.hpp
  HINTS ${MSGPACK_ROOT} $ENV{MSGPACK_ROOT}
  PATH_SUFFIXES include
)

find_path(MSGPACK_LIBRARY_DIR
  NAMES libmsgpackc.dylib libmsgpackc.so
  HINTS ${MSGPACK_ROOT} $ENV{MSGPACK_ROOT}
  PATH_SUFFIXES lib
)

find_library(MSGPACK_LIBRARY_SHARED
  NAMES libmsgpackc.dylib libmsgpackc.so
  HINTS ${MSGPACK_ROOT} $ENV{MSGPACK_ROOT}
  PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Msgpack
  REQUIRED_VARS MSGPACK_INCLUDE_DIR MSGPACK_LIBRARY_DIR MSGPACK_LIBRARY_SHARED
)

# idempotently import targets
if(NOT TARGET Msgpack AND Msgpack_FOUND)
  add_library(Msgpack SHARED IMPORTED)
  set_target_properties(Msgpack PROPERTIES
    IMPORTED_LOCATION ${MSGPACK_LIBRARY_SHARED}
    INTERFACE_INCLUDE_DIRECTORIES ${MSGPACK_INCLUDE_DIR}
  )
endif()
