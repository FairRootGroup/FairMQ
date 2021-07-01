################################################################################
#  Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

add_executable(fairmq-ex-dds-sampler sampler.cxx)
target_link_libraries(fairmq-ex-dds-sampler PRIVATE FairMQ)

add_executable(fairmq-ex-dds-processor processor.cxx)
target_link_libraries(fairmq-ex-dds-processor PRIVATE FairMQ)

add_executable(fairmq-ex-dds-sink sink.cxx)
target_link_libraries(fairmq-ex-dds-sink PRIVATE FairMQ)

add_custom_target(ExampleDDS DEPENDS fairmq-ex-dds-sampler fairmq-ex-dds-processor fairmq-ex-dds-sink)

list(JOIN Boost_LIBRARY_DIRS ":" LIB_DIR)
set(BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_BINARY_DIR}/fairmq/sdk)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology-infinite.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology-infinite.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-hosts.cfg ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-hosts.cfg COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-dds-env.sh ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-dds-env.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-dds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-dds.sh @ONLY)

# test
# if(DDS_FOUND)
  # add_test(NAME Example.DDS.localhost COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-dds.sh localhost)
  # set_tests_properties(Example.DDS.localhost PROPERTIES
    # TIMEOUT 15
    # PASS_REGULAR_EXPRESSION "Example successful"
  # )
# endif()

# install

install(
  TARGETS
  fairmq-ex-dds-sampler
  fairmq-ex-dds-processor
  fairmq-ex-dds-sink

  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(DATA_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology.xml_install @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology-infinite.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology-infinite.xml_install @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-dds-env.sh ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-dds-env.sh_install @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-dds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-dds.sh_install @ONLY)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology.xml_install
  DESTINATION ${PROJECT_INSTALL_DATADIR}
  RENAME ex-dds-topology.xml
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology-infinite.xml_install
  DESTINATION ${PROJECT_INSTALL_DATADIR}
  RENAME ex-dds-topology-infinite.xml
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-hosts.cfg
  DESTINATION ${PROJECT_INSTALL_DATADIR}
)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-dds-env.sh_install
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-ex-dds-env.sh
)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-dds.sh_install
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-dds.sh
)
