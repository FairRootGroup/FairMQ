################################################################################
#    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(plugin FairMQPlugin_pmix)
add_library(${plugin} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/PMIx.cxx ${CMAKE_CURRENT_SOURCE_DIR}/PMIx.h)
target_link_libraries(${plugin} FairMQ PMIx::libpmix)
target_include_directories(${plugin} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(${plugin} PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VERSION ${PROJECT_GIT_VERSION}
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

install(TARGETS ${plugin}
  EXPORT ${PROJECT_EXPORT_SET}
  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)
