################################################################################
#    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

#[=======================================================================[.rst:

``fairmq_target_tidy()``
========================

Runs the FairMQ static analyzer ``fairmq-tidy`` on source files in given
target.

.. code-block:: cmake

  fairmq_target_tidy(TARGET <target> [EXTRA_ARGS <args>]
                    [CLANG_EXECUTABLE <clang>])

Registers a custom command that depends on ``<target>`` which runs ``fairmq-tidy``
on the source files that belong to ``<target>``. Optional extra arguments
``<args>`` are passed to the ``fairmq-tidy`` command-line.

Requires ``CMAKE_EXPORT_COMPILE_COMMANDS`` to be enabled.

#]=======================================================================]

function(fairmq_target_tidy)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET;EXTRA_ARGS;CLANG_EXECUTABLE" "")

  if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
    message(AUTHOR_WARNING "CMAKE_EXPORT_COMPILE_COMMANDS is not enabled. Skipping.")
    return()
  endif()

  if(NOT ARG_TARGET)
    message(AUTHOR_WARNING "TARGET argument is required. Skipping.")
    return()
  endif()

  if(NOT TARGET ${ARG_TARGET})
    message(AUTHOR_WARNING "Given TARGET argument `${ARG_TARGET}` is not a target. Skipping.")
    return()
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(NOT fairmq_tidy_clang_resource_dir)
      if(ARG_CLANG_EXECUTABLE)
        set(clang_exe ${ARG_CLANG_EXECUTABLE})
      else()
        if(TARGET clang)
          get_property(clang_exe TARGET clang PROPERTY LOCATION)
        else()
          set(clang_exe "clang")
        endif()
      endif()
      execute_process(
        COMMAND ${clang_exe} -print-resource-dir
        OUTPUT_VARIABLE clang_resource_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      set(fairmq_tidy_clang_resource_dir ${clang_resource_dir}
        CACHE PATH "fairmq_target_tidy() internal state" FORCE)
    endif()
    list(APPEND ARG_EXTRA_ARGS
      "--extra-arg-before=-I${fairmq_tidy_clang_resource_dir}/include")
  endif()

  if(ARG_EXTRA_ARGS)
    set(extra 1)
  else()
    set(extra 0)
  endif()

  get_target_property(sources ${ARG_TARGET} SOURCES)
  list(FILTER sources INCLUDE REGEX "\.(cpp|cxx)$")
  string(REPLACE ":" "-" target_nocolon "${ARG_TARGET}")

  set(src_deps)
  foreach(source IN LISTS sources)
    string(REPLACE "\/" "-" source_noslash "${source}")
    set(src_stamp "${CMAKE_CURRENT_BINARY_DIR}/${target_nocolon}-${source_noslash}.fairmq-tidy")
    add_custom_command(
      OUTPUT ${src_stamp}
      COMMAND $<TARGET_FILE:FairMQ::fairmq-tidy> -p=${CMAKE_BINARY_DIR}
              $<${extra}:${ARG_EXTRA_ARGS}> ${source}
      COMMAND ${CMAKE_COMMAND} -E touch ${src_stamp}
      COMMENT "fairmq-tidy: Analyzing source file '${source}' of target '${ARG_TARGET}'"
      DEPENDS ${ARG_TARGET}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMAND_EXPAND_LISTS VERBATIM
    )
    list(APPEND src_deps ${src_stamp})
  endforeach()

  if(src_deps)
    add_custom_target("fairmq-tidy-${target_nocolon}" ALL DEPENDS ${src_deps})
  endif()
endfunction()
