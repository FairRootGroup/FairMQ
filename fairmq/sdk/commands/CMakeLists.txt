################################################################################
#    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(target Commands)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/CommandsFormat.h
  COMMAND $<TARGET_FILE:flatbuffers::flatc> -c -o ${CMAKE_CURRENT_BINARY_DIR} CommandsFormat.fbs
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/CommandsFormat_generated.h ${CMAKE_CURRENT_BINARY_DIR}/CommandsFormat.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# JSON serialization needs to see the .fbs file at run time, save it as constexpr string instead of locating/opening it every time
file(STRINGS CommandsFormat.fbs tmp)
list(JOIN tmp "\n" commands_format_def_fbs)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CommandsFormatDef.h.in ${CMAKE_CURRENT_BINARY_DIR}/CommandsFormatDef.h)

add_library(${target} Commands.cxx Commands.h ${CMAKE_CURRENT_BINARY_DIR}/CommandsFormat.h ${CMAKE_CURRENT_BINARY_DIR}/CommandsFormatDef.h)
add_library(FairMQ::${target} ALIAS ${target})

# Some distros may not package the static library (e.g. Fedora), so we pick up the dynamic library instead
set(_flatbuffers flatbuffers::flatbuffers)
if(NOT TARGET flatbuffers::flatbuffers AND TARGET flatbuffers::flatbuffers_shared)
  set(_flatbuffers flatbuffers::flatbuffers_shared)
endif()

target_link_libraries(${target}
  PUBLIC
  StateMachine
  Tools

  PRIVATE
  ${_flatbuffers}
)
target_compile_features(${target} PUBLIC cxx_std_17)
set_target_properties(${target} PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
  OUTPUT_NAME FairMQ${target}
)

target_include_directories(${target}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(
  TARGETS ${target}
  EXPORT ${PROJECT_EXPORT_SET}
  DESTINATION ${PROJECT_INSTALL_LIBDIR}
)

install(FILES Commands.h
  DESTINATION ${PROJECT_INSTALL_INCDIR}/sdk/commands
)
