################################################################################
#    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

add_executable(fairmq-ex-qc-sampler sampler.cxx)
target_link_libraries(fairmq-ex-qc-sampler PRIVATE FairMQ)

add_executable(fairmq-ex-qc-dispatcher qCDispatcher.cxx)
target_link_libraries(fairmq-ex-qc-dispatcher PRIVATE FairMQ)

add_executable(fairmq-ex-qc-task qCTask.cxx)
target_link_libraries(fairmq-ex-qc-task PRIVATE FairMQ)

add_executable(fairmq-ex-qc-sink sink.cxx)
target_link_libraries(fairmq-ex-qc-sink PRIVATE FairMQ)

add_custom_target(ExampleQC DEPENDS fairmq-ex-qc-sampler fairmq-ex-qc-dispatcher fairmq-ex-qc-task fairmq-ex-qc-sink)

list(JOIN Boost_LIBRARY_DIRS ":" LIB_DIR)
set(BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_BINARY_DIR}/fairmq/sdk)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-qc-topology.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-qc-topology.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-qc-env.sh ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-qc-env.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-qc.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-qc.sh @ONLY)

# test
if(DDS_FOUND)
  add_test(NAME Example.QC.localhost COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-qc.sh localhost)
  set_tests_properties(Example.QC.localhost PROPERTIES TIMEOUT 15 RUN_SERIAL true PASS_REGULAR_EXPRESSION "Example successful")
endif()

# install
install(
  TARGETS
  fairmq-ex-qc-sampler
  fairmq-ex-qc-dispatcher
  fairmq-ex-qc-task
  fairmq-ex-qc-sink

  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(DATA_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-qc-topology.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-qc-topology.xml_install @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-qc-env.sh ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-qc-env.sh_install @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-qc.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-qc.sh_install @ONLY)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/ex-qc-topology.xml_install
  DESTINATION ${PROJECT_INSTALL_DATADIR}
  RENAME ex-qc-topology.xml
)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-qc-env.sh_install
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-ex-qc-env.sh
)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-qc.sh_install
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-qc.sh
)
