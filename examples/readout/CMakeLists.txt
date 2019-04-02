 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

add_executable(fairmq-ex-readout-readout runReadout.cxx)
target_link_libraries(fairmq-ex-readout-readout PRIVATE FairMQ)

add_executable(fairmq-ex-readout-builder runBuilder.cxx)
target_link_libraries(fairmq-ex-readout-builder PRIVATE FairMQ)

add_executable(fairmq-ex-readout-processor runProcessor.cxx)
target_link_libraries(fairmq-ex-readout-processor PRIVATE FairMQ)

add_executable(fairmq-ex-readout-sender runSender.cxx)
target_link_libraries(fairmq-ex-readout-sender PRIVATE FairMQ)

add_executable(fairmq-ex-readout-receiver runReceiver.cxx)
target_link_libraries(fairmq-ex-readout-receiver PRIVATE FairMQ)

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-readout.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout.sh)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-readout-processing.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout-processing.sh)

# install

install(
    TARGETS
    fairmq-ex-readout-readout
    fairmq-ex-readout-builder
    fairmq-ex-readout-processor
    fairmq-ex-readout-sender
    fairmq-ex-readout-receiver

    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-readout.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout.sh_install)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-readout-processing.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout-processing.sh_install)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-start-ex-readout.sh
)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-readout-processing.sh_install
    DESTINATION ${PROJECT_INSTALL_BINDIR}
    RENAME fairmq-start-ex-readout-processing.sh
)
