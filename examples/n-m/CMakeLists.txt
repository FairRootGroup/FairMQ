################################################################################
#    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

add_executable(fairmq-ex-n-m-synchronizer synchronizer.cxx)
target_link_libraries(fairmq-ex-n-m-synchronizer PRIVATE FairMQ)

add_executable(fairmq-ex-n-m-sender sender.cxx)
target_link_libraries(fairmq-ex-n-m-sender PRIVATE FairMQ)

add_executable(fairmq-ex-n-m-receiver receiver.cxx)
target_link_libraries(fairmq-ex-n-m-receiver PRIVATE FairMQ)

add_custom_target(ExampleNM DEPENDS fairmq-ex-n-m-synchronizer fairmq-ex-n-m-sender fairmq-ex-n-m-receiver)

list(JOIN Boost_LIBRARY_DIRS ":" LIB_DIR)
set(BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-n-m-topology.xml                ${CMAKE_CURRENT_BINARY_DIR}/ex-n-m-topology.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-n-m-pair-topology.xml           ${CMAKE_CURRENT_BINARY_DIR}/ex-n-m-pair-topology.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-n-m-env.sh               ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-n-m-env.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-n-m.sh.in          ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-n-m.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-n-m-pair.sh.in     ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-n-m-pair.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-n-m-dds.sh.in      ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-n-m-dds.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-n-m-pair-dds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-n-m-pair-dds.sh @ONLY)

# install
install(
  TARGETS
  fairmq-ex-n-m-synchronizer
  fairmq-ex-n-m-sender
  fairmq-ex-n-m-receiver

  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure installed files with the install prefix known at install time
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/ex-n-m-topology.xml
  DESTINATION ${PROJECT_INSTALL_DATADIR}
  RENAME ex-n-m-topology.xml
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/ex-n-m-pair-topology.xml
  DESTINATION ${PROJECT_INSTALL_DATADIR}
  RENAME ex-n-m-pair-topology.xml
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-n-m-env.sh
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-ex-n-m-env.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-n-m.sh.in
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-n-m.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-n-m-pair.sh.in
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-n-m-pair.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-n-m-dds.sh.in
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-n-m-dds.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
fairmq_install_configured(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-n-m-pair-dds.sh.in
  PROGRAM
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-n-m-pair-dds.sh
  VARS BIN_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}"
       DATA_DIR "\${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR}"
       LIB_DIR "${LIB_DIR}"
)
