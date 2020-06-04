################################################################################
#    Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

find_path(PicoSHA2_INCLUDE_DIR NAMES picosha2.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PicoSHA2
  REQUIRED_VARS PicoSHA2_INCLUDE_DIR
)

if(PicoSHA2_FOUND)
  add_library(PicoSHA2 INTERFACE IMPORTED)
  set_target_properties(PicoSHA2 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PicoSHA2_INCLUDE_DIR}"
  )
endif()
