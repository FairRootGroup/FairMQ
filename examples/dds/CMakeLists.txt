################################################################################
#  Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

add_library(ExampleDDSLib STATIC
    "Sampler.cxx"
    "Sampler.h"
    "Processor.cxx"
    "Processor.h"
    "Sink.cxx"
    "Sink.h"
)

target_link_libraries(ExampleDDSLib PUBLIC FairMQ)

add_executable(fairmq-ex-dds-sampler runSampler.cxx)
target_link_libraries(fairmq-ex-dds-sampler PRIVATE ExampleDDSLib)

add_executable(fairmq-ex-dds-processor runProcessor.cxx)
target_link_libraries(fairmq-ex-dds-processor PRIVATE ExampleDDSLib)

add_executable(fairmq-ex-dds-sink runSink.cxx)
target_link_libraries(fairmq-ex-dds-sink PRIVATE ExampleDDSLib)

add_custom_target(ExampleDDS DEPENDS fairmq-ex-dds-sampler fairmq-ex-dds-processor fairmq-ex-dds-sink)

set(BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_BINARY_DIR}/fairmq/plugins/DDS)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology.xml @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-hosts.cfg ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-hosts.cfg COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-dds-env.sh ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-dds-env.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-dds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-dds.sh @ONLY)

# test

# install

install(
    TARGETS
    fairmq-ex-dds-sampler
    fairmq-ex-dds-processor
    fairmq-ex-dds-sink

    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(DATA_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ex-dds-topology.xml ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology.xml_install @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-ex-dds-env.sh ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-dds-env.sh_install @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-dds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-dds.sh_install @ONLY)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-topology.xml_install
    DESTINATION ${PROJECT_INSTALL_DATADIR}
    RENAME ex-dds-topology.xml
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/ex-dds-hosts.cfg
    DESTINATION ${PROJECT_INSTALL_DATADIR}
)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-ex-dds-env.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-ex-dds-env.sh
)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-dds.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-start-ex-dds.sh
)
