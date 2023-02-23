 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

add_executable(fairmq-ex-multiple-channels-sampler sampler.cxx)
target_link_libraries(fairmq-ex-multiple-channels-sampler PRIVATE FairMQ)

add_executable(fairmq-ex-multiple-channels-broadcaster broadcaster.cxx)
target_link_libraries(fairmq-ex-multiple-channels-broadcaster PRIVATE FairMQ)

add_executable(fairmq-ex-multiple-channels-sink sink.cxx)
target_link_libraries(fairmq-ex-multiple-channels-sink PRIVATE FairMQ)

add_custom_target(ExampleMultipleChannels DEPENDS fairmq-ex-multiple-channels-sampler fairmq-ex-multiple-channels-broadcaster fairmq-ex-multiple-channels-sink)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-multiple-channels.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multiple-channels.sh)

# test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-ex-multiple-channels.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test-ex-multiple-channels.sh)

add_test(NAME Example.MultipleChannels.zeromq COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-multiple-channels.sh zeromq)
set_tests_properties(Example.MultipleChannels.zeromq PROPERTIES TIMEOUT "30")

# install

install(
  TARGETS
  fairmq-ex-multiple-channels-sampler
  fairmq-ex-multiple-channels-broadcaster
  fairmq-ex-multiple-channels-sink

  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-multiple-channels.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multiple-channels.sh_install)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multiple-channels.sh_install
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-multiple-channels.sh
)
