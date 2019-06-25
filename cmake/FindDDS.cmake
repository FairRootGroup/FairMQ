################################################################################
# Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

find_path(DDS_INCLUDE_DIR
  NAMES dds_intercom.h
  HINTS ${DDS_ROOT} $ENV{DDS_ROOT}
  PATH_SUFFIXES include include/DDS
)

find_path(DDS_LIBRARY_DIR
  NAMES libdds_intercom_lib.dylib libdds_intercom_lib.so
  HINTS ${DDS_ROOT} $ENV{DDS_ROOT}
  PATH_SUFFIXES lib
)

find_library(DDS_INTERCOM_LIBRARY_SHARED
  NAMES libdds_intercom_lib.dylib libdds_intercom_lib.so
  HINTS ${DDS_ROOT} $ENV{DDS_ROOT}
  PATH_SUFFIXES lib
  DOC "Path to libdds_intercom_lib.dylib libdds_intercom_lib.so."
)

find_library(DDS_PROTOCOL_LIBRARY_SHARED
  NAMES libdds_protocol_lib.dylib libdds_protocol_lib.so
  HINTS ${DDS_ROOT} $ENV{DDS_ROOT}
  PATH_SUFFIXES lib
  DOC "Path to libdds_protocol_lib.dylib libdds_protocol_lib.so."
)

find_library(DDS_USER_DEFAULTS_LIBRARY_SHARED
  NAMES libdds-user-defaults.dylib libdds-user-defaults.so
  HINTS ${DDS_ROOT} $ENV{DDS_ROOT}
  PATH_SUFFIXES lib
  DOC "Path to libdds-user-defaults.dylib libdds-user-defaults.so."
)

find_file(DDS_VERSION_FILE
  NAMES version
  HINTS ${DDS_ROOT} $ENV{DDS_ROOT}
  PATH_SUFFIXES etc
)

if(DDS_VERSION_FILE AND NOT DDS_VERSION)
  file(READ ${DDS_VERSION_FILE} DDS_VERSION)
  string(STRIP "${DDS_VERSION}" DDS_VERSION)
  set(DDS_VERSION ${DDS_VERSION} CACHE string "DDS version.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DDS
  REQUIRED_VARS
  DDS_INCLUDE_DIR
  DDS_LIBRARY_DIR
  DDS_INTERCOM_LIBRARY_SHARED
  DDS_PROTOCOL_LIBRARY_SHARED
  DDS_USER_DEFAULTS_LIBRARY_SHARED

  VERSION_VAR DDS_VERSION
)

if(NOT TARGET DDS::dds_intercom_lib AND DDS_FOUND)
  add_library(DDS::dds_intercom_lib SHARED IMPORTED)
  set_target_properties(DDS::dds_intercom_lib PROPERTIES
    IMPORTED_LOCATION ${DDS_INTERCOM_LIBRARY_SHARED}
    INTERFACE_INCLUDE_DIRECTORIES ${DDS_INCLUDE_DIR}
  )
endif()

if(NOT TARGET DDS::dds_protocol_lib AND DDS_FOUND)
  add_library(DDS::dds_protocol_lib SHARED IMPORTED)
  set_target_properties(DDS::dds_protocol_lib PROPERTIES
    IMPORTED_LOCATION ${DDS_PROTOCOL_LIBRARY_SHARED}
    INTERFACE_INCLUDE_DIRECTORIES ${DDS_INCLUDE_DIR}
  )
endif()

if(NOT TARGET DDS::dds-user-defaults AND DDS_FOUND)
  add_library(DDS::dds-user-defaults SHARED IMPORTED)
  set_target_properties(DDS::dds-user-defaults PROPERTIES
    IMPORTED_LOCATION ${DDS_USER_DEFAULTS_LIBRARY_SHARED}
    INTERFACE_INCLUDE_DIRECTORIES ${DDS_INCLUDE_DIR}
  )
endif()
