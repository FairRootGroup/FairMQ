################################################################################
#    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

################
# libFairMQSDK #
################
set(target SDK)

set(SDK_PUBLIC_HEADER_FILES
  Session.h
  Topology.h
)

set(SDK_PRIVATE_HEADER_FILES
)

set(SDK_SOURCE_FILES
  Session.cxx
  Topology.cxx
)

add_library(${target}
  ${SDK_SOURCE_FILES}
  ${SDK_PUBLIC_HEADER_FILES} # for IDE integration
  ${SDK_PRIVATE_HEADER_FILES} # for IDE integration
)
set_target_properties(${target} PROPERTIES LABELS coverage)
target_compile_definitions(${target} PUBLIC BOOST_ERROR_CODE_HEADER_ONLY)
target_include_directories(${target}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${target}
  PUBLIC
  FairLogger::FairLogger
  StateMachine

  PRIVATE
  Tools
  DDS::dds_intercom_lib
  DDS::dds_protocol_lib
)
set_target_properties(${target} PROPERTIES
  VERSION ${PROJECT_GIT_VERSION}
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
  OUTPUT_NAME FairMQ_${target}
)

###############
# executables #
###############
add_executable(fairmq runFairMQ.cxx)
target_link_libraries(fairmq
  PRIVATE
  SDK
  Boost::program_options
)

install(
  TARGETS
  SDK
  fairmq

  EXPORT ${PROJECT_EXPORT_SET}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${PROJECT_INSTALL_LIBDIR}
)

# preserve relative path and prepend fairmq
foreach(HEADER IN LISTS SDK_PUBLIC_HEADER_FILES)
  get_filename_component(_path ${HEADER} DIRECTORY)
  file(TO_CMAKE_PATH ${PROJECT_INSTALL_INCDIR}/sdk/${_path} _destination)
  install(FILES ${HEADER}
    DESTINATION ${_destination}
  )
endforeach()
