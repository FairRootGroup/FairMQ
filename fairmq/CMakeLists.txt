################################################################################
# Copyright (C) 2012-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################
#
# This is the top-level cmake file for the FairMQ submodule.
#
# Testing:
#   Enable the building of tests by enabling the cmake option BUILD_TESTING.
#
# Linking:
#   Depend on FairMQ target, if you want to link against libFairMQ.
#
# Convenience targets defined:
#   * FairMQFull    - build everything in the submodule except tests
#   * FairMQAll     - build everything including tests, if enabled
#   * FairMQInstall - install everything from the FairMQ submodule
#   * FairMQTests   - build all tests in the submodule
#
#

############################
# preprocessor definitions #
############################
if(NANOMSG_FOUND)
    add_definitions(-DNANOMSG_FOUND)
    if(MSGPACK_FOUND)
        add_definitions(-DMSGPACK_FOUND)
    endif()
endif()


##################
# subdirectories #
##################
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
if(DDS_FOUND)
    add_subdirectory(plugins/config)
    add_subdirectory(plugins/control)
endif(DDS_FOUND)


##########################
# libFairMQ header files #
##########################
set(FAIRMQ_HEADER_FILES
    FairMQChannel.h
    FairMQConfigPlugin.h
    FairMQConfigurable.h
    FairMQControlPlugin.h
    FairMQDevice.h
    FairMQLogger.h
    FairMQMessage.h
    FairMQParts.h
    FairMQPoller.h
    FairMQSocket.h
    FairMQStateMachine.h
    FairMQTransportFactory.h
    FairMQTransports.h
    devices/FairMQBenchmarkSampler.h
    devices/FairMQMerger.h
    devices/FairMQMultiplier.h
    devices/FairMQProxy.h
    devices/FairMQSink.h
    devices/FairMQSplitter.h
    logger/logger.h
    logger/logger_def.h
    options/FairMQEventManager.h
    options/FairMQParser.h
    options/FairMQProgOptions.h
    options/FairMQSuboptParser.h
    options/FairProgOptions.h
    options/FairProgOptionsHelper.h
    runFairMQDevice.h
    shmem/FairMQContextSHM.h
    shmem/FairMQMessageSHM.h
    shmem/FairMQPollerSHM.h
    shmem/FairMQSocketSHM.h
    shmem/FairMQTransportFactorySHM.h
    tools/FairMQTools.h
    tools/runSimpleMQStateMachine.h
    zeromq/FairMQContextZMQ.h
    zeromq/FairMQMessageZMQ.h
    zeromq/FairMQPollerZMQ.h
    zeromq/FairMQSocketZMQ.h
    zeromq/FairMQTransportFactoryZMQ.h
)

if(NANOMSG_FOUND)
    set(FAIRMQ_HEADER_FILES ${FAIRMQ_HEADER_FILES}
        nanomsg/FairMQMessageNN.h
        nanomsg/FairMQPollerNN.h
        nanomsg/FairMQSocketNN.h
        nanomsg/FairMQTransportFactoryNN.h
    )
endif()

if("${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}" VERSION_LESS "1.56")
    set(FAIRMQ_HEADER_FILES_NAMESPACED ${FAIRMQ_HEADER_FILES_NAMESPACED}
        logger/fairroot_null_deleter.h
    )
endif()


set(FAIRMQ_SOURCE_FILES
    FairMQChannel.cxx
    FairMQConfigurable.cxx
    FairMQDevice.cxx
    FairMQLogger.cxx
    FairMQMessage.cxx
    FairMQPoller.cxx
    FairMQSocket.cxx
    FairMQStateMachine.cxx
    FairMQTransportFactory.cxx
    devices/FairMQBenchmarkSampler.cxx
    devices/FairMQMerger.cxx
    devices/FairMQMultiplier.cxx
    devices/FairMQProxy.cxx
    devices/FairMQSink.cxx
    devices/FairMQSplitter.cxx
    logger/logger.cxx
    options/FairMQParser.cxx
    options/FairMQProgOptions.cxx
    options/FairMQSuboptParser.cxx
    options/FairProgOptions.cxx
    shmem/FairMQContextSHM.cxx
    shmem/FairMQMessageSHM.cxx
    shmem/FairMQPollerSHM.cxx
    shmem/FairMQSocketSHM.cxx
    shmem/FairMQTransportFactorySHM.cxx
    zeromq/FairMQContextZMQ.cxx
    zeromq/FairMQMessageZMQ.cxx
    zeromq/FairMQPollerZMQ.cxx
    zeromq/FairMQSocketZMQ.cxx
    zeromq/FairMQTransportFactoryZMQ.cxx
)

if(NANOMSG_FOUND)
    set(FAIRMQ_SOURCE_FILES ${FAIRMQ_SOURCE_FILES}
        nanomsg/FairMQMessageNN.cxx
        nanomsg/FairMQPollerNN.cxx
        nanomsg/FairMQSocketNN.cxx
        nanomsg/FairMQTransportFactoryNN.cxx
    )
endif()

###################
# configure files #
###################
configure_file(${CMAKE_SOURCE_DIR}/fairmq/run/startMQBenchmark.sh.in ${CMAKE_BINARY_DIR}/bin/startMQBenchmark.sh)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/run/benchmark.json ${CMAKE_BINARY_DIR}/bin/config/benchmark.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/options/startConfigExample.sh.in
               ${CMAKE_BINARY_DIR}/bin/startConfigExample.sh)


#################################
# define libFairMQ build target #
#################################
add_library(FairMQ SHARED
    ${FAIRMQ_SOURCE_FILES}
    ${FAIRMQ_HEADER_FILES} # for IDE integration
)


#######################
# include directories #
#######################
target_include_directories(FairMQ
    PUBLIC # consumers inherit public include directories
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/fairmq>
)


##################
# link libraries #
##################
target_link_libraries(FairMQ
    INTERFACE # only consumers link against interface dependencies
    dl

    PUBLIC # libFairMQ AND consumers of libFairMQ link aginst public dependencies
    pthread
    Boost::boost
    Boost::log
    Boost::log_setup
    Boost::program_options
    Boost::thread
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
    
    PRIVATE # only libFairMQ links against private dependencies
    ZeroMQ
    $<$<BOOL:${NANOMSG_FOUND}>:nanomsg>
    $<$<AND:$<BOOL:${NANOMSG_FOUND}>,$<BOOL:${MSGPACK_FOUND}>>:Msgpack>
)


###############
# executables #
###############
add_executable(bsampler run/runBenchmarkSampler.cxx)
target_link_libraries(bsampler FairMQ)

add_executable(merger run/runMerger.cxx)
target_link_libraries(merger FairMQ)

add_executable(multiplier run/runMultiplier.cxx)
target_link_libraries(multiplier FairMQ)

add_executable(proxy run/runProxy.cxx)
target_link_libraries(proxy FairMQ)

add_executable(sink run/runSink.cxx)
target_link_libraries(sink FairMQ)

add_executable(splitter run/runSplitter.cxx)
target_link_libraries(splitter FairMQ)

add_executable(runConfigExample options/runConfigEx.cxx)
target_link_libraries(runConfigExample FairMQ)


####################
# aggregate target #
####################
# all targets except tests
set(FAIRMQ_FULL_TARGETS
    FairMQ
    bsampler
    merger
    multiplier
    proxy
    sink
    splitter
)
add_custom_target(FairMQFull DEPENDS ${FAIRMQ_FULL_TARGETS})
# all targets including tests, if enabled
if(BUILD_TESTING)
    set(FAIRMQ_TEST_TARGET FairMQTests)
endif()
add_custom_target(FairMQAll
    DEPENDS
    FairMQFull
    ${FAIRMQ_TEST_TARGET}
)


###########################
# generate cotire targets #
###########################
#cotire(${FAIRMQ_FULL_TARGETS})
# disabled by default for now, because it messes up target properties
# still useful for development


###########
# install #
###########
install(
    TARGETS # FairMQFull, tests are not installed
    ${FAIRMQ_FULL_TARGETS}

    EXPORT FairMQ

    LIBRARY
    DESTINATION lib
    COMPONENT fairmq

    RUNTIME
    DESTINATION bin
    COMPONENT fairmq
)

# preserve relative path and prepend fairmq
foreach(HEADER ${FAIRMQ_HEADER_FILES})
    get_filename_component(_path ${HEADER} DIRECTORY)
    file(TO_CMAKE_PATH include/fairmq/${_path} _destination)
    if("${_path}" STREQUAL "logger")
        install(FILES ${HEADER}
            DESTINATION include/logger
            COMPONENT fairmq
        )
    else()
        install(FILES ${HEADER}
            DESTINATION ${_destination}
            COMPONENT fairmq
        )
    endif()
endforeach()

# export FairMQ targets
install(
    EXPORT FairMQ
    DESTINATION include/cmake
    NAMESPACE FairRoot::
    EXPORT_LINK_INTERFACE_LIBRARIES
    COMPONENT fairmq
)

# use the following target to only install the fairmq component
add_custom_target(FairMQInstall
    DEPENDS FairMQFull
    COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_COMPONENT=fairmq -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
)


