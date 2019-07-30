################################################################################
#    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(_quiet_option)
if(Ncurses_FIND_QUIETLY)
  set(_quiet_option "QUIET")
endif()

set(_version)
if(Ncurses_FIND_VERSION)
  if(Ncurses_FIND_VERSION_EXACT)
    set(_version "==${Ncurses_FIND_VERSION}")
  else()
    set(_version ">=${Ncurses_FIND_VERSION}")
  endif()
endif()

find_package(PkgConfig QUIET REQUIRED)

# Find main library
pkg_check_modules(Ncurses ${_quiet_option} ncurses${_version})

if(Ncurses_FOUND AND NOT TARGET Ncurses::ncurses)
  add_library(Ncurses::ncurses INTERFACE IMPORTED)
  set_target_properties(Ncurses::ncurses PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Ncurses_INCLUDEDIR}"
    INTERFACE_LINK_DIRECTORIES "${Ncurses_LIBDIR}"
    INTERFACE_LINK_OPTIONS "${Ncurses_LDFLAGS}"
  )
  if(Ncurses_CFLAGS)
    set_target_properties(Ncurses::ncurses PROPERTIES
      INTERFACE_COMPILE_OPTIONS "${Ncurses_CFLAGS}"
    )
  endif()
endif()

# Find components
foreach(_comp IN LISTS Ncurses_FIND_COMPONENTS)
  pkg_check_modules(Ncurses_${_comp} ${_quiet_option} ${_comp}${_version})

  if(Ncurses_${_comp}_FOUND AND NOT TARGET Ncurses::${_comp})
    add_library(Ncurses::${_comp} INTERFACE IMPORTED)
    set_target_properties(Ncurses::${_comp} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Ncurses_${_comp}_INCLUDEDIR}"
      INTERFACE_LINK_DIRECTORIES "${Ncurses_${_comp}_LIBDIR}"
      INTERFACE_LINK_OPTIONS "${Ncurses_${_comp}_LDFLAGS}"
    )
    if(Ncurses_${_comp}_CFLAGS)
      set_target_properties(Ncurses::${_comp} PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${Ncurses_${_comp}_CFLAGS}"
      )
    endif()
  endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ncurses
  REQUIRED_VARS Ncurses_FOUND
  VERSION_VAR Ncurses_VERSION
  HANDLE_COMPONENTS
)
