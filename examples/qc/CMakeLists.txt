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
set(BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-qc-topology.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-qc-topology.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-qc-env.sh ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-qc-env.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-qc.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-qc.sh @ONLY)

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

# configure installed files with the install prefix known at install time
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/ex-qc-topology.xml
  DESTINATION ${PROJECT_INSTALL_DATADIR}
  RENAME ex-qc-topology.xml
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-qc-env.sh
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-ex-qc-env.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-qc.sh.in
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-qc.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
