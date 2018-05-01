################################################################################
#    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

add_library(ExampleCopyPushLib STATIC
    "Sampler.cxx"
    "Sampler.h"
    "Sink.cxx"
    "Sink.h"
)

target_link_libraries(ExampleCopyPushLib PUBLIC FairMQ)

add_executable(fairmq-ex-copypush-sampler runSampler.cxx)
target_link_libraries(fairmq-ex-copypush-sampler PRIVATE ExampleCopyPushLib)


add_executable(fairmq-ex-copypush-sink runSink.cxx)
target_link_libraries(fairmq-ex-copypush-sink PRIVATE ExampleCopyPushLib)

add_custom_target(ExampleCopyPush DEPENDS fairmq-ex-copypush-sampler fairmq-ex-copypush-sink)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-copypush.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-copypush.sh)

# test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-ex-copypush.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test-ex-copypush.sh)

add_test(NAME Example-CopyPush COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-copypush.sh)
set_tests_properties(Example-CopyPush PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Received message: ")

# install

install(
    TARGETS
    fairmq-ex-copypush-sampler
    fairmq-ex-copypush-sink

    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-copypush.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-copypush.sh_install)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-copypush.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-start-ex-copypush.sh
)
