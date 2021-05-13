 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

add_executable(fairmq-ex-multipart-sampler sampler.cxx)
target_link_libraries(fairmq-ex-multipart-sampler PRIVATE FairMQ)

add_executable(fairmq-ex-multipart-sink sink.cxx)
target_link_libraries(fairmq-ex-multipart-sink PRIVATE FairMQ)

add_custom_target(ExampleMultipart DEPENDS fairmq-ex-multipart-sampler fairmq-ex-multipart-sink)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-multipart.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multipart.sh)

# test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-ex-multipart.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test-ex-multipart.sh)

add_test(NAME Example.Multipart.zeromq COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-multipart.sh zeromq)
set_tests_properties(Example.Multipart.zeromq PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Received message with 7 parts")

add_test(NAME Example.Multipart.shmem COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-multipart.sh shmem)
set_tests_properties(Example.Multipart.shmem PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Received message with 7 parts")

if(BUILD_OFI_TRANSPORT)
  add_test(NAME Example.Multipart.ofi COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-multipart.sh ofi)
  set_tests_properties(Example.Multipart.ofi PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Received message with 7 parts")
endif()

# install

install(
  TARGETS
  fairmq-ex-multipart-sampler
  fairmq-ex-multipart-sink

  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-multipart.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multipart.sh_install)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multipart.sh_install
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-multipart.sh
)
