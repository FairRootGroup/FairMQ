################################################################################
# Copyright (C) 2012-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(plugin FairMQPlugin_dds)
add_library(${plugin} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/DDS.cxx ${CMAKE_CURRENT_SOURCE_DIR}/DDS.h)
target_link_libraries(${plugin} FairMQ DDS::dds_intercom_lib DDS::dds_protocol_lib)
target_include_directories(${plugin} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(${plugin} PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(${plugin} PROPERTIES
  VERSION ${PROJECT_GIT_VERSION}
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

set(exe fairmq-dds-command-ui)
add_executable(${exe} ${CMAKE_CURRENT_SOURCE_DIR}/runDDSCommandUI.cxx)
target_link_libraries(${exe} FairMQ DDS::dds_intercom_lib DDS::dds_protocol_lib)
target_include_directories(${exe} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS ${plugin} ${exe}
  EXPORT ${PROJECT_EXPORT_SET}
  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)
