################################################################################
# Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

find_path(nanomsg_INCLUDE_DIR
  NAMES nanomsg/nn.h
  HINTS ${NANOMSG_ROOT} $ENV{NANOMSG_ROOT}
  PATH_SUFFIXES include
  DOC "Path to nanomsg include header files."
)

find_library(nanomsg_LIBRARY_SHARED
  NAMES libnanomsg.dylib libnanomsg.so
  HINTS ${NANOMSG_ROOT} $ENV{NANOMSG_ROOT}
  PATH_SUFFIXES lib
  DOC "Path to libnanomsg.dylib libnanomsg.so."
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nanomsg
  REQUIRED_VARS nanomsg_LIBRARY_SHARED nanomsg_INCLUDE_DIR
)

if(NOT TARGET nanomsg AND nanomsg_FOUND)
  add_library(nanomsg SHARED IMPORTED)
  set_target_properties(nanomsg PROPERTIES
    IMPORTED_LOCATION ${nanomsg_LIBRARY_SHARED}
    INTERFACE_INCLUDE_DIRECTORIES ${nanomsg_INCLUDE_DIR}
  )
endif()
