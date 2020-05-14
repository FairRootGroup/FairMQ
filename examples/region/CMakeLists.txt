 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

add_library(ExampleRegionLib STATIC
    "Sampler.cxx"
    "Sampler.h"
    "Sink.cxx"
    "Sink.h"
)

target_link_libraries(ExampleRegionLib PUBLIC FairMQ)

add_executable(fairmq-ex-region-sampler runSampler.cxx)
target_link_libraries(fairmq-ex-region-sampler PRIVATE ExampleRegionLib)

add_executable(fairmq-ex-region-sink runSink.cxx)
target_link_libraries(fairmq-ex-region-sink PRIVATE ExampleRegionLib)

add_custom_target(ExampleRegion DEPENDS fairmq-ex-region-sampler fairmq-ex-region-sink)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-region.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-region.sh)

# test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-ex-region.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test-ex-region.sh)

add_test(NAME Example.Region.zeromq COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-region.sh zeromq)
set_tests_properties(Example.Region.zeromq PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Received [0-9*] acks")

add_test(NAME Example.Region.shmem COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-region.sh shmem)
set_tests_properties(Example.Region.shmem PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Received [0-9*] acks")

# install

install(
    TARGETS
    fairmq-ex-region-sampler
    fairmq-ex-region-sink

    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-region.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-region.sh_install)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-region.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-start-ex-region.sh
)
