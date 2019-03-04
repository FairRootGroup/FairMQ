 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

add_library(ExampleMultipleTransportsLib STATIC
    "Sampler1.cxx"
    "Sampler1.h"
    "Sampler2.cxx"
    "Sampler2.h"
    "Sink.cxx"
    "Sink.h"
)

target_link_libraries(ExampleMultipleTransportsLib PUBLIC FairMQ)

add_executable(fairmq-ex-multiple-transports-sampler1 runSampler1.cxx)
target_link_libraries(fairmq-ex-multiple-transports-sampler1 PRIVATE ExampleMultipleTransportsLib)

add_executable(fairmq-ex-multiple-transports-sampler2 runSampler2.cxx)
target_link_libraries(fairmq-ex-multiple-transports-sampler2 PRIVATE ExampleMultipleTransportsLib)

add_executable(fairmq-ex-multiple-transports-sink runSink.cxx)
target_link_libraries(fairmq-ex-multiple-transports-sink PRIVATE ExampleMultipleTransportsLib)

add_custom_target(ExampleMultipleTransports DEPENDS fairmq-ex-multiple-transports-sampler1 fairmq-ex-multiple-transports-sampler2 fairmq-ex-multiple-transports-sink)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-multiple-transports.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multiple-transports.sh)

# test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-ex-multiple-transports.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test-ex-multiple-transports.sh)
add_test(NAME Example.MultipleTransports COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-multiple-transports.sh)
set_tests_properties(Example.MultipleTransports PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Received messages from both sources.")

# install

install(
    TARGETS
    fairmq-ex-multiple-transports-sampler1
    fairmq-ex-multiple-transports-sampler2
    fairmq-ex-multiple-transports-sink

    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-multiple-transports.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multiple-transports.sh_install)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-multiple-transports.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-start-ex-multiple-transports.sh
)
