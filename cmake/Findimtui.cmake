################################################################################
#    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(pkg imtui)
find_path(${pkg}_INCLUDE_DIR
  NAMES ${pkg}.h
  PATH_SUFFIXES include/${pkg}
  DOC "imtui include directory"
)
get_filename_component(${pkg}_INCLUDE_DIR "${${pkg}_INCLUDE_DIR}/.." ABSOLUTE)

find_library(${pkg}_LIBRARY
  NAMES lib${pkg}.so
  PATH_SUFFIXES lib lib64
  DOC "Path to libimtui.a"
)

find_library(${pkg}_ncurses_LIBRARY
  NAMES lib${pkg}-ncurses.so
  PATH_SUFFIXES lib lib64
  DOC "Path to libimtui-ncurses.a"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${pkg} REQUIRED_VARS
  ${pkg}_INCLUDE_DIR
  ${pkg}_LIBRARY
  ${pkg}_ncurses_LIBRARY
)

get_filename_component(${pkg}_PREFIX "${${pkg}_INCLUDE_DIR}/.." ABSOLUTE)

if(${pkg}_FOUND AND NOT TARGET ${pkg}::${pkg}-ncurses)
  add_library(${pkg}::${pkg}-ncurses SHARED IMPORTED)
  set_target_properties(${pkg}::${pkg}-ncurses PROPERTIES
    IMPORTED_LOCATION ${${pkg}_ncurses_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${${pkg}_INCLUDE_DIR}
  )
endif()

if(${pkg}_FOUND AND NOT TARGET ${pkg}::${pkg})
  add_library(${pkg}::${pkg} SHARED IMPORTED)
  set_target_properties(${pkg}::${pkg} PROPERTIES
    IMPORTED_LOCATION ${${pkg}_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${${pkg}_INCLUDE_DIR}
  )
endif()

mark_as_advanced(
  ${pkg}_INCLUDE_DIR
  ${pkg}_LIBRARY
  ${pkg}_ncurses_LIBRARY
)
