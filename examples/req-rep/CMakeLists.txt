 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

add_executable(fairmq-ex-req-rep-client client.cxx)
target_link_libraries(fairmq-ex-req-rep-client PRIVATE FairMQ)


add_executable(fairmq-ex-req-rep-server server.cxx)
target_link_libraries(fairmq-ex-req-rep-server PRIVATE FairMQ)

add_custom_target(ExampleReqRep DEPENDS fairmq-ex-req-rep-client fairmq-ex-req-rep-server)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-req-rep.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-req-rep.sh)

# test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-ex-req-rep.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test-ex-req-rep.sh)

add_test(NAME Example.ReqRep.zeromq COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-req-rep.sh zeromq)
set_tests_properties(Example.ReqRep.zeromq PROPERTIES TIMEOUT "30" PASS_REGULAR_EXPRESSION "Received reply from server: ")

add_test(NAME Example.ReqRep.shmem COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-req-rep.sh shmem)
set_tests_properties(Example.ReqRep.shmem PROPERTIES TIMEOUT "30" PASS_REGULAR_EXPRESSION "Received reply from server: ")

# install

install(
  TARGETS
  fairmq-ex-req-rep-client
  fairmq-ex-req-rep-server

  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-req-rep.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-req-rep.sh_install)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-req-rep.sh_install
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-req-rep.sh
)
