################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

# According to the docs the modification of the PKG_CONFIG_PATH environment should
# not be necessary, but it does not work otherwise.
if(OFI_ROOT)
  list(APPEND CMAKE_PREFIX_PATH "${OFI_ROOT}/lib/pkgconfig")
  set(ENV{PKG_CONFIG_PATH} "${OFI_ROOT}/lib/pkgconfig:" $ENV{PKG_CONFIG_PATH})
endif()

if(ENV{OFI_ROOT})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{OFI_ROOT}/lib/pkgconfig")
  set(ENV{PKG_CONFIG_PATH} "$ENV{OFI_ROOT}/lib/pkgconfig:" $ENV{PKG_CONFIG_PATH})
endif()

# This should be the default as of CMake 3.1, but it is not set. BUG? Also, it does not work
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH 1)
find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  # Find include dir and dependencies from pkgconfig
  pkg_check_modules(_OFI libfabric QUIET)

  # Retrieve version from pkgconfig
  execute_process(
    COMMAND ${PKG_CONFIG_EXECUTABLE} libfabric --modversion
    OUTPUT_VARIABLE OFI_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # The IMPORTED_TARGET option of the pkg_check_modules() function is useless,
  # so let's build it ourselves
  find_library(OFI_LIBFABRIC
    NAMES libfabric.so libfabric.dylib
    HINTS ${OFI_ROOT} $ENV{OFI_ROOT}
    PATH_SUFFIXES lib
  )

  # Just take the include dirs found by the PkgConfig module
  set(OFI_INCLUDE_DIRS ${_OFI_INCLUDE_DIRS})

  # Find fi_info command to be able to check required features of the OFI installation
  find_program(OFI_INFO_EXECUTABLE
    NAMES fi_info
    HINTS ${OFI_ROOT} $ENV{OFI_ROOT}
    PATH_SUFFIXES bin
  )

  # Detect ofi providers, they can be required via the COMPONENTS argument of find_package
  if(OFI_INFO_EXECUTABLE)
    execute_process(
      COMMAND ${OFI_INFO_EXECUTABLE} -l
      OUTPUT_VARIABLE output
    )
    string(REPLACE "\n" ";" lines ${output})
    foreach(line IN LISTS lines)
      string(REGEX
        MATCH "^([a-zA-Z0-9_]+):"
        found "${line}"
      )
      if(found)
        string(TOLOWER "${CMAKE_MATCH_1}" provider)
        set(OFI_fi_${provider}_FOUND TRUE)
      endif()
    endforeach()
  endif()

  # Check search result, check version constraints and print status
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(OFI
    REQUIRED_VARS OFI_LIBFABRIC OFI_INCLUDE_DIRS OFI_INFO_EXECUTABLE
    VERSION_VAR OFI_VERSION
    HANDLE_COMPONENTS
  )
endif()

if(NOT TARGET OFI::libfabric AND OFI_FOUND)
  # Define an imported target
  add_library(OFI::libfabric SHARED IMPORTED GLOBAL)
  set_target_properties(OFI::libfabric PROPERTIES
    IMPORTED_LOCATION ${OFI_LIBFABRIC}
    INTERFACE_INCLUDE_DIRECTORIES ${OFI_INCLUDE_DIRS}
  )
endif()
