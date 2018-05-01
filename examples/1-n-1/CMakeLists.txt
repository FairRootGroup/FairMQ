################################################################################
#    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

add_library(Example1N1Lib STATIC
    "Sampler.cxx"
    "Sampler.h"
    "Processor.cxx"
    "Processor.h"
    "Sink.cxx"
    "Sink.h"
)

target_link_libraries(Example1N1Lib PUBLIC FairMQ)

add_executable(fairmq-ex-1-n-1-sampler runSampler.cxx)
target_link_libraries(fairmq-ex-1-n-1-sampler PRIVATE Example1N1Lib)

add_executable(fairmq-ex-1-n-1-processor runProcessor.cxx)
target_link_libraries(fairmq-ex-1-n-1-processor PRIVATE Example1N1Lib)

add_executable(fairmq-ex-1-n-1-sink runSink.cxx)
target_link_libraries(fairmq-ex-1-n-1-sink PRIVATE Example1N1Lib)

add_custom_target(Example1N1 DEPENDS fairmq-ex-1-n-1-sampler fairmq-ex-1-n-1-processor fairmq-ex-1-n-1-sink)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(EX_CONF_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-1-n-1.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-1-n-1.sh)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-1-n-1.json ${CMAKE_CURRENT_BINARY_DIR}/ex-1-n-1.json)

# test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-ex-1-n-1.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test-ex-1-n-1.sh)

add_test(NAME Example-1-n-1 COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-1-n-1.sh)
set_tests_properties(Example-1-n-1 PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Received: ")

# install

install(
    TARGETS
    fairmq-ex-1-n-1-sampler
    fairmq-ex-1-n-1-processor
    fairmq-ex-1-n-1-sink

    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(EX_CONF_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-1-n-1.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-1-n-1.sh_install)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-1-n-1.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-start-ex-1-n-1.sh
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/ex-1-n-1.json
    DESTINATION ${PROJECT_INSTALL_DATADIR}
)
