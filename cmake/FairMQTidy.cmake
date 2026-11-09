################################################################################
# Copyright (C) 2021-2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
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

  fairmq_target_tidy(TARGET <target> [CHECKS <globs>] [EXTRA_ARGS <args>...])

Registers a custom command that depends on ``<target>`` which runs ``fairmq-tidy``
on the source files that belong to ``<target>``. ``fairmq-tidy`` is ``clang-tidy``
with the ``fairmq-*`` checks built in.

``CHECKS`` selects the checks (default: ``fairmq-*``). It replaces the
``Checks`` of any ``.clang-tidy`` file. Optional extra arguments ``<args>`` are
passed to the ``fairmq-tidy`` command-line, e.g. ``--fix`` or
``--warnings-as-errors=fairmq-*``.

The argument ``CLANG_EXECUTABLE`` of earlier versions has no effect:
``fairmq-tidy`` knows the Clang resource directory it was built with.

Requires ``CMAKE_EXPORT_COMPILE_COMMANDS`` to be enabled.

#]=======================================================================]

function(fairmq_target_tidy)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET;CHECKS;CLANG_EXECUTABLE" "EXTRA_ARGS")

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

  if(NOT ARG_CHECKS)
    set(ARG_CHECKS "fairmq-*")
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
      COMMAND $<TARGET_FILE:FairMQ::fairmq-tidy> -p=${PROJECT_BINARY_DIR}
              "--checks=-*,${ARG_CHECKS}" ${ARG_EXTRA_ARGS} ${source}
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
