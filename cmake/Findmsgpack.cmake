################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

unset(_args)

if(msgpack_FIND_VERSION)
  list(APPEND _args ${msgpack_FIND_VERSION})
endif()

if(msgpack_FIND_EXACT)
  list(APPEND _args "EXACT")
endif()

if(msgpack_FIND_QUIETLY)
  list(APPEND _args "QUIET")
endif()

if(msgpack_FIND_REQUIRED)
  list(APPEND _args "REQUIRED")
endif()

if(msgpack_FIND_COMPONENTS)
  list(APPEND _args "COMPONENTS" ${msgpack_FIND_COMPONENTS})
endif()

find_package(msgpack ${_args} CONFIG)

if(msgpack_FOUND AND NOT TARGET msgpack::msgpack)
  # config mode find_package does not set $msgpack_ROOT, workaround by extracting
  # root path from library target
  unset(_msgpack_lib)
  unset(_prefix)
  get_target_property(_msgpack_lib msgpackc INTERFACE_LOCATION)
  get_filename_component(_prefix ${_msgpack_lib} DIRECTORY)
  get_filename_component(_prefix ${_prefix}/.. ABSOLUTE)

  add_library(msgpack::msgpack INTERFACE IMPORTED)
  set_target_properties(msgpack::msgpack PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_prefix}/include"
  )
endif()
