################################################################################
#    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

find_path(PMIx_INCLUDE_DIR
  NAMES pmix.h
  HINTS ${PMIX_ROOT} $ENV{PMIX_ROOT}
  PATH_SUFFIXES include
)

find_path(PMIx_LIBRARY_DIR
  NAMES libpmix.dylib libpmix.so
  HINTS ${PMIX_ROOT} $ENV{PMIX_ROOT}
  PATH_SUFFIXES lib lib64
)

find_library(PMIx_LIBRARY_SHARED
  NAMES libpmix.dylib libpmix.so
  HINTS ${PMIX_ROOT} $ENV{PMIX_ROOT}
  PATH_SUFFIXES lib lib64
)

find_file(PMIx_VERSION_FILE
  NAMES pmix_version.h
  HINTS ${PMIX_ROOT} $ENV{PMIX_ROOT}
  PATH_SUFFIXES include
)

file(READ "${PMIx_VERSION_FILE}" __version_raw)
string(REGEX MATCH "#define PMIX_VERSION_MAJOR ([0-9]?)L?"
  __version_major_raw "${__version_raw}"
)
set(PMIx_VERSION_MAJOR "${CMAKE_MATCH_1}")

string(REGEX MATCH "#define PMIX_VERSION_MINOR ([0-9]?)L?"
  __version_minor_raw "${__version_raw}"
)
set(PMIx_VERSION_MINOR "${CMAKE_MATCH_1}")

string(REGEX MATCH "#define PMIX_VERSION_RELEASE ([0-9]?)L?"
  __version_patch_raw "${__version_raw}"
)
set(PMIx_VERSION_PATCH "${CMAKE_MATCH_1}")

set(PMIx_VERSION "${PMIx_VERSION_MAJOR}.${PMIx_VERSION_MINOR}.${PMIx_VERSION_PATCH}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PMIx
  REQUIRED_VARS
  PMIx_INCLUDE_DIR
  PMIx_LIBRARY_DIR
  PMIx_LIBRARY_SHARED

  VERSION_VAR PMIx_VERSION
)

if(NOT TARGET PMIx::libpmix AND PMIx_FOUND)
  add_library(PMIx::libpmix SHARED IMPORTED)
  set_target_properties(PMIx::libpmix PROPERTIES
    IMPORTED_LOCATION ${PMIx_LIBRARY_SHARED}
    INTERFACE_INCLUDE_DIRECTORIES ${PMIx_INCLUDE_DIR}
  )
endif()
