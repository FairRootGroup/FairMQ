################################################################################
# Copyright (C) 2019-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(plugin FairMQPlugin_pmix)
add_library(${plugin} SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/PMIxPlugin.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/PMIxPlugin.h
  ${CMAKE_CURRENT_SOURCE_DIR}/PMIxCommands.h
  ${CMAKE_CURRENT_SOURCE_DIR}/PMIx.hpp
)
target_compile_features(${plugin} PUBLIC cxx_std_17)
target_link_libraries(${plugin} PUBLIC FairMQ PMIx::libpmix)
target_include_directories(${plugin} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(${plugin} PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VERSION ${PROJECT_VERSION}
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

set(exe fairmq-pmix-command-ui)
add_executable(${exe} ${CMAKE_CURRENT_SOURCE_DIR}/runPMIxCommandUI.cxx)
target_link_libraries(${exe} FairMQ StateMachine PMIx::libpmix)
target_include_directories(${exe} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS ${plugin} ${exe}
  EXPORT ${PROJECT_EXPORT_SET}
  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)
