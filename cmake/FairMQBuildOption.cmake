################################################################################
# Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

include(CMakeDependentOption)

macro(fairmq_build_option option description)
  cmake_parse_arguments(ARGS "" "DEFAULT" "REQUIRES" ${ARGN})

  if(ARGS_DEFAULT)
    set(__default__ ON)
  else()
    set(__default__ OFF)
  endif()

  if(ARGS_REQUIRES)
    set(__requires__ ${ARGS_REQUIRES})
    foreach(d ${__requires__})
      string(REGEX REPLACE " +" ";" __requires_condition__ "${d}")
      if(${__requires_condition__})
      else()
        if(${option})
          message(FATAL_ERROR "Cannot enable build option ${option}, depending option is not set: ${__requires_condition__}")
        endif()
      endif()
    endforeach()
  else()
    set(__requires__)
  endif()

  if(__requires__)
    cmake_dependent_option(${option} ${description} ${__default__} "${__requires__}" OFF)
  else()
    option(${option} ${description} ${__default__})
  endif()

  set(__default__)
  set(__requires__)
  set(__requires_condition__)
  set(ARGS_DEFAULT)
  set(ARGS_REQUIRES)
  set(option)
  set(description)
endmacro()
