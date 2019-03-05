 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

add_library(ExampleReadoutLib STATIC
    "Sampler.cxx"
    "Sampler.h"
    "Builder.h"
    "Sink.cxx"
    "Sink.h"
)

target_link_libraries(ExampleReadoutLib PUBLIC FairMQ)

add_executable(fairmq-ex-readout-sampler runSampler.cxx)
target_link_libraries(fairmq-ex-readout-sampler PRIVATE ExampleReadoutLib)

add_executable(fairmq-ex-readout-builder runBuilder.cxx)
target_link_libraries(fairmq-ex-readout-builder PRIVATE ExampleReadoutLib)

add_executable(fairmq-ex-readout-sink runSink.cxx)
target_link_libraries(fairmq-ex-readout-sink PRIVATE ExampleReadoutLib)

add_custom_target(ExampleReadout DEPENDS fairmq-ex-readout-sampler fairmq-ex-readout-builder fairmq-ex-readout-sink)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-readout.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout.sh)

# install

install(
    TARGETS
    fairmq-ex-readout-sampler
    fairmq-ex-readout-builder
    fairmq-ex-readout-sink

    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-readout.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout.sh_install)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-start-ex-readout.sh
)
