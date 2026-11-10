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
set(BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology-infinite.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology-infinite.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-hosts.cfg ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-hosts.cfg COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-dds-env.sh ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-dds-env.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-dds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-dds.sh @ONLY)


# install

install(
  TARGETS
  fairmq-ex-dds-sampler
  fairmq-ex-dds-processor
  fairmq-ex-dds-sink

  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-hosts.cfg
  DESTINATION ${PROJECT_INSTALL_DATADIR}
)

# configure installed files with the install prefix known at install time
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology.xml
  DESTINATION ${PROJECT_INSTALL_DATADIR}
  RENAME ex-dds-topology.xml
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology-infinite.xml
  DESTINATION ${PROJECT_INSTALL_DATADIR}
  RENAME ex-dds-topology-infinite.xml
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-dds-env.sh
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-ex-dds-env.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-dds.sh.in
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-dds.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
