################################################################################
# Copyright (C) 2012-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

####################
# external plugins #
####################
if(BUILD_DDS_PLUGIN)
  add_subdirectory(plugins/DDS)
endif()
if(BUILD_PMIX_PLUGIN)
  add_subdirectory(plugins/PMIx)
endif()


############################
# preprocessor definitions #
############################
if(BUILD_NANOMSG_TRANSPORT)
  add_definitions(-DBUILD_NANOMSG_TRANSPORT)
endif()
if(BUILD_OFI_TRANSPORT)
  add_definitions(-DBUILD_OFI_TRANSPORT)
endif()


##################
# subdirectories #
##################
if(BUILD_OFI_TRANSPORT)
  add_subdirectory(ofi)
endif()


##########################
# libFairMQ header files #
##########################
set(FAIRMQ_PUBLIC_HEADER_FILES
  DeviceRunner.h
  EventManager.h
  FairMQChannel.h
  FairMQDevice.h
  FairMQLogger.h
  FairMQMessage.h
  FairMQParts.h
  FairMQPoller.h
  FairMQUnmanagedRegion.h
  FairMQSocket.h
  FairMQStateMachine.h
  FairMQTransportFactory.h
  MemoryResources.h
  MemoryResourceTools.h
  Tools.h
  Transports.h
  options/FairMQProgOptions.h
  options/FairProgOptions.h
  Plugin.h
  PluginManager.h
  PluginServices.h
  runFairMQDevice.h
  tools/CppSTL.h
  tools/Network.h
  tools/Process.h
  tools/RateLimit.h
  tools/Strings.h
  tools/Unique.h
  tools/Version.h
)

set(FAIRMQ_PRIVATE_HEADER_FILES
  devices/FairMQBenchmarkSampler.h
  devices/FairMQMerger.h
  devices/FairMQMultiplier.h
  devices/FairMQProxy.h
  devices/FairMQSink.h
  devices/FairMQSplitter.h
  options/FairMQParser.h
  options/FairMQSuboptParser.h
  options/FairProgOptionsHelper.h
  plugins/Builtin.h
  plugins/Control.h
  StateMachine.h
  shmem/FairMQMessageSHM.h
  shmem/FairMQPollerSHM.h
  shmem/FairMQUnmanagedRegionSHM.h
  shmem/FairMQSocketSHM.h
  shmem/FairMQTransportFactorySHM.h
  shmem/Common.h
  shmem/Manager.h
  shmem/Region.h
  zeromq/FairMQMessageZMQ.h
  zeromq/FairMQPollerZMQ.h
  zeromq/FairMQUnmanagedRegionZMQ.h
  zeromq/FairMQSocketZMQ.h
  zeromq/FairMQTransportFactoryZMQ.h
)

if(BUILD_NANOMSG_TRANSPORT)
  set(FAIRMQ_PRIVATE_HEADER_FILES ${FAIRMQ_PRIVATE_HEADER_FILES}
    nanomsg/FairMQMessageNN.h
    nanomsg/FairMQPollerNN.h
    nanomsg/FairMQUnmanagedRegionNN.h
    nanomsg/FairMQSocketNN.h
    nanomsg/FairMQTransportFactoryNN.h
  )
endif()

if(BUILD_OFI_TRANSPORT)
  set(FAIRMQ_PRIVATE_HEADER_FILES ${FAIRMQ_PRIVATE_HEADER_FILES}
    ofi/Context.h
    ofi/Message.h
    ofi/Poller.h
    ofi/Socket.h
    ofi/TransportFactory.h
  )
endif()

##########################
# libFairMQ source files #
##########################
set(FAIRMQ_SOURCE_FILES
  DeviceRunner.cxx
  FairMQChannel.cxx
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
  devices/FairMQSplitter.cxx
  options/FairMQParser.cxx
  options/FairMQProgOptions.cxx
  options/FairMQSuboptParser.cxx
  Plugin.cxx
  PluginManager.cxx
  PluginServices.cxx
  plugins/Control.cxx
  StateMachine.cxx
  shmem/FairMQMessageSHM.cxx
  shmem/FairMQPollerSHM.cxx
  shmem/FairMQUnmanagedRegionSHM.cxx
  shmem/FairMQSocketSHM.cxx
  shmem/FairMQTransportFactorySHM.cxx
  shmem/Manager.cxx
  shmem/Region.cxx
  tools/Network.cxx
  tools/Process.cxx
  tools/Unique.cxx
  zeromq/FairMQMessageZMQ.cxx
  zeromq/FairMQPollerZMQ.cxx
  zeromq/FairMQUnmanagedRegionZMQ.cxx
  zeromq/FairMQSocketZMQ.cxx
  zeromq/FairMQTransportFactoryZMQ.cxx
  MemoryResources.cxx
)

if(BUILD_NANOMSG_TRANSPORT)
  set(FAIRMQ_SOURCE_FILES ${FAIRMQ_SOURCE_FILES}
    nanomsg/FairMQMessageNN.cxx
    nanomsg/FairMQPollerNN.cxx
    nanomsg/FairMQUnmanagedRegionNN.cxx
    nanomsg/FairMQSocketNN.cxx
    nanomsg/FairMQTransportFactoryNN.cxx
  )
endif()

if(BUILD_OFI_TRANSPORT)
  set(FAIRMQ_SOURCE_FILES ${FAIRMQ_SOURCE_FILES}
    ofi/Context.cxx
    ofi/Message.cxx
    ofi/Poller.cxx
    ofi/Socket.cxx
    ofi/TransportFactory.cxx
  )
endif()


###################
# configure files #
###################
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run/startMQBenchmark.sh.in ${CMAKE_CURRENT_BINARY_DIR}/startMQBenchmark.sh)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/options/startConfigExample.sh.in ${CMAKE_CURRENT_BINARY_DIR}/startConfigExample.sh)


#################################
# define libFairMQ build target #
#################################
if(FAST_BUILD)
  set(_target FairMQ_)
else()
  set(_target FairMQ)
endif()
add_library(${_target}
  ${FAIRMQ_SOURCE_FILES}
  ${FAIRMQ_PUBLIC_HEADER_FILES} # for IDE integration
  ${FAIRMQ_PRIVATE_HEADER_FILES} # for IDE integration
)
set_target_properties(${_target} PROPERTIES LABELS coverage)
if(FAST_BUILD)
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME FairMQ)
endif()

#######################
# include directories #
#######################
target_include_directories(${_target}
  PUBLIC # consumers inherit public include directories
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
  $<INSTALL_INTERFACE:include/fairmq>
  $<INSTALL_INTERFACE:include>
)

##################
# link libraries #
##################
if(BUILD_NANOMSG_TRANSPORT)
  set(NANOMSG_DEPS nanomsg msgpackc-cxx)
endif()
if(BUILD_OFI_TRANSPORT)
  set(OFI_DEPS OFI::libfabric protobuf::libprotobuf $<TARGET_OBJECTS:OfiTransport>)
endif()
set(optional_deps ${NANOMSG_DEPS} ${OFI_DEPS})
if(optional_deps)
  list(REMOVE_DUPLICATES optional_deps)
endif()

target_link_libraries(${_target}
  INTERFACE # only consumers link against interface dependencies
  Boost::container

  PUBLIC # libFairMQ AND consumers of libFairMQ link aginst public dependencies
  Threads::Threads
  dl
  $<$<PLATFORM_ID:Linux>:rt>
  Boost::boost
  Boost::program_options
  Boost::thread
  Boost::system
  Boost::filesystem
  Boost::regex
  Boost::date_time
  FairLogger::FairLogger

  PRIVATE # only libFairMQ links against private dependencies
  libzmq
  ${NANOMSG_DEPS}
  ${OFI_DEPS}
)
set_target_properties(${_target} PROPERTIES
  VERSION ${PROJECT_GIT_VERSION}
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)


##############
# fast build #
##############
if(FAST_BUILD)
  set_target_properties(${_target} PROPERTIES
    COTIRE_UNITY_TARGET_NAME "FairMQ"
    # COTIRE_ENABLE_PRECOMPILED_HEADER FALSE
    EXCLUDE_FROM_ALL TRUE
  )
  cotire(${_target})
  set_target_properties(FairMQ PROPERTIES EXCLUDE_FROM_ALL FALSE)
  set_target_properties(FairMQ PROPERTIES LABELS coverage)
endif()


###############
# executables #
###############
add_executable(fairmq-bsampler run/runBenchmarkSampler.cxx)
target_link_libraries(fairmq-bsampler FairMQ)

add_executable(fairmq-merger run/runMerger.cxx)
target_link_libraries(fairmq-merger FairMQ)

add_executable(fairmq-multiplier run/runMultiplier.cxx)
target_link_libraries(fairmq-multiplier FairMQ)

add_executable(fairmq-proxy run/runProxy.cxx)
target_link_libraries(fairmq-proxy FairMQ)

add_executable(fairmq-sink run/runSink.cxx)
target_link_libraries(fairmq-sink FairMQ)

add_executable(fairmq-splitter run/runSplitter.cxx)
target_link_libraries(fairmq-splitter FairMQ)

add_executable(runConfigExample options/runConfigEx.cxx)
target_link_libraries(runConfigExample FairMQ)

add_executable(fairmq-shmmonitor shmem/Monitor.cxx shmem/Monitor.h shmem/runMonitor.cxx)
target_link_libraries(fairmq-shmmonitor PUBLIC
  Threads::Threads
  $<$<PLATFORM_ID:Linux>:rt>
  Boost::boost
  Boost::date_time
  Boost::program_options
)
target_include_directories(fairmq-shmmonitor PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

add_executable(fairmq-uuid-gen run/runUuidGenerator.cxx)
target_link_libraries(fairmq-uuid-gen FairMQ)


###########
# install #
###########
install(
  TARGETS
  FairMQ
  fairmq-bsampler
  fairmq-merger
  fairmq-multiplier
  fairmq-proxy
  fairmq-sink
  fairmq-splitter
  fairmq-shmmonitor
  fairmq-uuid-gen

  EXPORT ${PROJECT_EXPORT_SET}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${PROJECT_INSTALL_LIBDIR}
)

# preserve relative path and prepend fairmq
foreach(HEADER ${FAIRMQ_PUBLIC_HEADER_FILES})
  get_filename_component(_path ${HEADER} DIRECTORY)
  file(TO_CMAKE_PATH ${PROJECT_INSTALL_INCDIR}/${_path} _destination)
  install(FILES ${HEADER}
    DESTINATION ${_destination}
  )
endforeach()
