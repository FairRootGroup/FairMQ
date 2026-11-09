################################################################################
# Copyright (C) 2018-2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

find_package(Git QUIET)

# fairmq_init_submodule(<path>)
#
# Checks out the Git submodule <path> (relative to PROJECT_SOURCE_DIR) if its
# folder is empty. Stops with an error if the folder stays empty.
function(fairmq_init_submodule path)
  set(dir "${PROJECT_SOURCE_DIR}/${path}")
  file(GLOB content "${dir}/*")
  if(content)
    return()
  endif()

  if(GIT_EXECUTABLE AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    message(STATUS "Checking out submodule ${path}")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init -- ${path}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE log
      ERROR_VARIABLE log
      RESULT_VARIABLE res
    )
    file(GLOB content "${dir}/*")
  endif()

  if(NOT content)
    message(FATAL_ERROR "The bundled package in ${path} is missing. "
                        "Run `git submodule update --init` in ${PROJECT_SOURCE_DIR}, "
                        "or install the package. ${log}")
  endif()
endfunction()
