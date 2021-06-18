################################################################################
# Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

if(BUILD_TIDY_TOOL)
  include(FairMQTidy)
endif()

#################
# libFairMQ_SDK #
#################
configure_file(DDSInfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/DDSInfo.h @ONLY)

set(target SDK)

set(SDK_PUBLIC_HEADER_FILES
  ../SDK.h
  AsioAsyncOp.h
  AsioBase.h
  DDSAgent.h
  DDSCollection.h
  DDSEnvironment.h
  DDSSession.h
  DDSTask.h
  DDSTopology.h
  Error.h
  Topology.h
  Traits.h
)

set(SDK_PRIVATE_HEADER_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/DDSInfo.h
)

set(SDK_SOURCE_FILES
  DDSEnvironment.cxx
  DDSSession.cxx
  DDSTopology.cxx
  Topology.cxx
)

add_library(${target}
  ${SDK_SOURCE_FILES}
  ${SDK_PUBLIC_HEADER_FILES} # for IDE integration
  ${SDK_PRIVATE_HEADER_FILES} # for IDE integration
)
target_compile_features(${target} PUBLIC cxx_std_17)
set_target_properties(${target} PROPERTIES LABELS coverage)
target_include_directories(${target}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${target}
  PUBLIC
  asio::asio
  Boost::boost
  Boost::filesystem
  FairLogger::FairLogger
  Threads::Threads
  Tools
  StateMachine
  Commands

  PRIVATE
  DDS::dds_intercom_lib
  DDS::dds_tools_lib
  DDS::dds_topology_lib
)
set_target_properties(${target} PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
  OUTPUT_NAME FairMQ_${target}
)
if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
  fairmq_target_tidy(TARGET ${target})
endif()

###############
# executables #
###############
set(target fairmq-dds-command-ui)
add_executable(${target} ${CMAKE_CURRENT_SOURCE_DIR}/runDDSCommandUI.cxx)
target_link_libraries(${target} PRIVATE FairMQ Commands SDK StateMachine)
target_compile_features(${target} PRIVATE cxx_std_17)
if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
  fairmq_target_tidy(TARGET ${target})
endif()

if(BUILD_SDK_TOP_TOOL)
  set(fairmq_top "fairmq-top")
  add_executable(${fairmq_top}
    top/runTool.cxx
    top/Tool.h
  )
  target_link_libraries(${fairmq_top} PRIVATE
    SDK asio::asio CLI11::CLI11 imtui::imtui imtui::imtui-ncurses)
  target_compile_features(${fairmq_top} PRIVATE cxx_std_17)
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET ${fairmq_top})
  endif()
else()
  set(fairmq_top)
endif()

install(
  TARGETS
  SDK
  fairmq-dds-command-ui
  ${fairmq_top}

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

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/DDSInfo.h
  DESTINATION ${PROJECT_INSTALL_INCDIR}/sdk
)
