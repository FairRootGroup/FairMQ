################################################################################
# Copyright (C) 2012-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(plugin FairMQPlugin_dds)

add_library(${plugin} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/DDS.cxx ${CMAKE_CURRENT_SOURCE_DIR}/DDS.h)
target_link_libraries(${plugin} FairMQ ${DDS_INTERCOM_LIBRARY_SHARED} ${DDS_PROTOCOL_LIBRARY_SHARED} ${DDS_USER_DEFAULTS_LIBRARY_SHARED})
target_include_directories(${plugin} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} PUBLIC ${DDS_INCLUDE_DIR})
set_target_properties(${plugin} PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_executable(fairmq-dds-command-ui ${CMAKE_CURRENT_SOURCE_DIR}/runDDSCommandUI.cxx)
target_link_libraries(fairmq-dds-command-ui FairMQ ${DDS_INTERCOM_LIBRARY_SHARED} ${DDS_PROTOCOL_LIBRARY_SHARED} ${DDS_USER_DEFAULTS_LIBRARY_SHARED})
target_include_directories(fairmq-dds-command-ui PRIVATE ${CMAKE_CURRENT_BINARY_DIR} PUBLIC ${DDS_INCLUDE_DIR})

