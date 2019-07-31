################################################################################
#    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

find_path(asio_INCLUDE_DIR
  NAMES asio.hpp
  PATH_SUFFIXES include
)

if(asio_INCLUDE_DIR)
  find_file(asio_VERSION_HEADER "asio/version.hpp"
    ${asio_INCLUDE_DIR}
    NO_DEFAULT_PATH
  )
endif()

if(asio_VERSION_HEADER)
  file(READ "${asio_VERSION_HEADER}" _asio_VERSION_HEADER_CONTENT)
  string(REGEX MATCH "#define ASIO_VERSION ([0-9]+)" _MATCH "${_asio_VERSION_HEADER_CONTENT}")
  set(asio_VERSION_MACRO ${CMAKE_MATCH_1})
  math(EXPR asio_VERSION_MAJOR "${asio_VERSION_MACRO} / 100000")
  math(EXPR asio_VERSION_MINOR "${asio_VERSION_MACRO} / 100 % 1000")
  math(EXPR asio_VERSION_PATCH "${asio_VERSION_MACRO} % 100")
  set(asio_VERSION "${asio_VERSION_MAJOR}.${asio_VERSION_MINOR}.${asio_VERSION_PATCH}")
endif()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(asio
  REQUIRED_VARS asio_INCLUDE_DIR
  VERSION_VAR asio_VERSION
  HANDLE_COMPONENTS
)

if(asio_FOUND AND asio_BUNDLED)
  add_library(bundled_asio_headers INTERFACE)
  target_include_directories(bundled_asio_headers INTERFACE
    $<BUILD_INTERFACE:${asio_BUILD_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${asio_INSTALL_INCLUDE_DIR}>
  )
  add_library(asio::headers ALIAS bundled_asio_headers)
endif()
if(asio_FOUND AND NOT TARGET asio::headers)
  add_library(asio::headers INTERFACE IMPORTED)
  set_target_properties(asio::headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${asio_INCLUDE_DIR}"
  )
endif()

