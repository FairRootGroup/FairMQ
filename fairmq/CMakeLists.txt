################################################################################
# Copyright (C) 2012-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

if(BUILD_FAIRMQ OR BUILD_SDK)
  ###########
  # Version #
  ###########
  configure_file(Version.h.in
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/Version.h
    @ONLY
  )
  install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/Version.h
    DESTINATION ${PROJECT_INSTALL_INCDIR}
  )

  #########
  # Tools #
  #########
  set(target Tools)

  set(TOOLS_PUBLIC_HEADER_FILES
    tools/CppSTL.h
    tools/InstanceLimit.h
    tools/Network.h
    tools/Process.h
    tools/RateLimit.h
    tools/Semaphore.h
    tools/Strings.h
    tools/Unique.h
    tools/Version.h
    Tools.h
  )

  set(TOOLS_SOURCE_FILES
    tools/Network.cxx
    tools/Process.cxx
    tools/Semaphore.cxx
    tools/Unique.cxx
  )

  add_library(${target}
    ${TOOLS_SOURCE_FILES}
    ${TOOLS_PUBLIC_HEADER_FILES}
  )
  target_compile_features(${target} PUBLIC cxx_std_17)
  target_compile_definitions(${target} PUBLIC BOOST_ERROR_CODE_HEADER_ONLY)
  target_include_directories(${target}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(${target}
    PRIVATE
    FairLogger::FairLogger
    Threads::Threads
    Boost::boost
    PUBLIC
    asio::asio
  )
  set_target_properties(${target} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    OUTPUT_NAME FairMQ${target}
  )
  install(
    TARGETS ${target}
    EXPORT ${PROJECT_EXPORT_SET}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${PROJECT_INSTALL_LIBDIR}
  )
  foreach(HEADER ${TOOLS_PUBLIC_HEADER_FILES})
    get_filename_component(_path ${HEADER} DIRECTORY)
    file(TO_CMAKE_PATH ${PROJECT_INSTALL_INCDIR}/${_path} _destination)
    install(FILES ${HEADER}
      DESTINATION ${_destination}
    )
  endforeach()

  #################
  # State Machine #
  #################
  set(target StateMachine)

  set(FSM_PUBLIC_HEADER_FILES
    StateMachine.h
    States.h
    StateQueue.h
  )

  set(FSM_SOURCE_FILES
    StateMachine.cxx
    States.cxx
  )

  add_library(${target}
    ${FSM_SOURCE_FILES}
    ${FSM_PUBLIC_HEADER_FILES}
  )
  target_compile_features(${target} PUBLIC cxx_std_17)
  target_compile_definitions(${target} PUBLIC BOOST_ERROR_CODE_HEADER_ONLY)
  target_include_directories(${target}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(${target}
    PUBLIC
    FairLogger::FairLogger

    PRIVATE
    Boost::boost
    Tools
  )
  set_target_properties(${target} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    OUTPUT_NAME FairMQ${target}
  )
  install(
    TARGETS ${target}
    EXPORT ${PROJECT_EXPORT_SET}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${PROJECT_INSTALL_LIBDIR}
  )
  foreach(HEADER ${FSM_PUBLIC_HEADER_FILES})
    get_filename_component(_path ${HEADER} DIRECTORY)
    file(TO_CMAKE_PATH ${PROJECT_INSTALL_INCDIR}/${_path} _destination)
    install(FILES ${HEADER}
      DESTINATION ${_destination}
    )
  endforeach()
endif()

if(BUILD_FAIRMQ)
  ##########################
  # libFairMQ header files #
  ##########################
  set(FAIRMQ_PUBLIC_HEADER_FILES
    Channel.h
    Device.h
    DeviceRunner.h
    EventManager.h
    FairMQChannel.h
    FairMQDevice.h
    FairMQLogger.h
    FairMQMessage.h
    FairMQParts.h
    FairMQPoller.h
    FairMQSocket.h
    FairMQTransportFactory.h
    FairMQUnmanagedRegion.h
    JSONParser.h
    MemoryResourceTools.h
    MemoryResources.h
    Message.h
    Plugin.h
    PluginManager.h
    PluginServices.h
    Poller.h
    ProgOptions.h
    ProgOptionsFwd.h
    Properties.h
    PropertyOutput.h
    SuboptParser.h
    TransportFactory.h
    Transports.h
    UnmanagedRegion.h
    options/FairMQProgOptions.h
    runDevice.h
    runFairMQDevice.h
    shmem/Monitor.h
  )

  set(FAIRMQ_PRIVATE_HEADER_FILES
    devices/BenchmarkSampler.h
    devices/Merger.h
    devices/Multiplier.h
    devices/Proxy.h
    devices/Sink.h
    devices/Splitter.h
    plugins/Builtin.h
    plugins/config/Config.h
    plugins/control/Control.h
    shmem/Message.h
    shmem/Poller.h
    shmem/UnmanagedRegion.h
    shmem/Socket.h
    shmem/TransportFactory.h
    shmem/Common.h
    shmem/Manager.h
    shmem/Region.h
    zeromq/Context.h
    zeromq/Message.h
    zeromq/Poller.h
    zeromq/UnmanagedRegion.h
    zeromq/Socket.h
    zeromq/TransportFactory.h
  )

  if(BUILD_OFI_TRANSPORT)
    set(FAIRMQ_PRIVATE_HEADER_FILES ${FAIRMQ_PRIVATE_HEADER_FILES}
      ofi/Context.h
      ofi/ControlMessages.h
      ofi/Message.h
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
    FairMQTransportFactory.cxx
    Plugin.cxx
    PluginManager.cxx
    PluginServices.cxx
    ProgOptions.cxx
    JSONParser.cxx
    Properties.cxx
    SuboptParser.cxx
    plugins/config/Config.cxx
    plugins/control/Control.cxx
    MemoryResources.cxx
    shmem/Manager.cxx
    shmem/Monitor.cxx
  )

  if(BUILD_OFI_TRANSPORT)
    set(FAIRMQ_SOURCE_FILES ${FAIRMQ_SOURCE_FILES}
      ofi/Context.cxx
      ofi/Message.cxx
      ofi/Socket.cxx
    )
  endif()


  ###################
  # configure files #
  ###################
  set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/devices/startMQBenchmark.sh.in ${CMAKE_CURRENT_BINARY_DIR}/startMQBenchmark.sh)

  #################################
  # define libFairMQ build target #
  #################################
  set(target FairMQ)
  add_library(${target}
    ${FAIRMQ_SOURCE_FILES}
    ${FAIRMQ_PUBLIC_HEADER_FILES} # for IDE integration
    ${FAIRMQ_PRIVATE_HEADER_FILES} # for IDE integration
  )
  target_compile_features(${target} PUBLIC cxx_std_17)
  set_target_properties(${target} PROPERTIES LABELS coverage)


  ############################
  # preprocessor definitions #
  ############################
  target_compile_definitions(${target} PUBLIC BOOST_ERROR_CODE_HEADER_ONLY)
  if(FAIRMQ_DEBUG_MODE)
    target_compile_definitions(${target} PUBLIC FAIRMQ_DEBUG_MODE)
  endif()
  if(BUILD_OFI_TRANSPORT)
    target_compile_definitions(${target} PRIVATE BUILD_OFI_TRANSPORT)
  endif()
  target_compile_definitions(${target} PUBLIC FAIRMQ_HAS_STD_FILESYSTEM=${FAIRMQ_HAS_STD_FILESYSTEM})


  #######################
  # include directories #
  #######################
  target_include_directories(${target}
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
  if(BUILD_OFI_TRANSPORT)
    set(OFI_DEPS
      asiofi::asiofi
    )
  endif()

  target_link_libraries(${target}
    INTERFACE # only consumers link against interface dependencies
    Boost::container

    PUBLIC # libFairMQ AND consumers of libFairMQ link aginst public dependencies
    Threads::Threads
    dl
    $<$<PLATFORM_ID:Linux>:rt>
    Boost::boost
    Boost::program_options
    Boost::filesystem
    Boost::regex
    FairLogger::FairLogger
    Tools
    StateMachine

    PRIVATE # only libFairMQ links against private dependencies
    libzmq
    PicoSHA2
    ${OFI_DEPS}
  )
  set_target_properties(${target} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
  )


  ###############
  # executables #
  ###############
  add_executable(fairmq-bsampler devices/runBenchmarkSampler.cxx)
  target_link_libraries(fairmq-bsampler FairMQ)

  add_executable(fairmq-merger devices/runMerger.cxx)
  target_link_libraries(fairmq-merger FairMQ)

  add_executable(fairmq-multiplier devices/runMultiplier.cxx)
  target_link_libraries(fairmq-multiplier FairMQ)

  add_executable(fairmq-proxy devices/runProxy.cxx)
  target_link_libraries(fairmq-proxy FairMQ)

  add_executable(fairmq-sink devices/runSink.cxx)
  target_link_libraries(fairmq-sink FairMQ)

  add_executable(fairmq-splitter devices/runSplitter.cxx)
  target_link_libraries(fairmq-splitter FairMQ)

  add_executable(fairmq-shmmonitor shmem/Monitor.cxx shmem/Monitor.h shmem/runMonitor.cxx)
  target_compile_features(fairmq-shmmonitor PUBLIC cxx_std_17)
  target_compile_definitions(fairmq-shmmonitor PUBLIC BOOST_ERROR_CODE_HEADER_ONLY)
  if(FAIRMQ_DEBUG_MODE)
    target_compile_definitions(fairmq-shmmonitor PUBLIC FAIRMQ_DEBUG_MODE)
  endif()
  target_link_libraries(fairmq-shmmonitor PUBLIC
    Threads::Threads
    $<$<PLATFORM_ID:Linux>:rt>
    Boost::boost
    Boost::date_time
    $<$<NOT:${FAIRMQ_HAS_STD_FILESYSTEM}>:Boost::filesystem>
    Boost::program_options
    FairLogger::FairLogger
    PicoSHA2
  )
  target_include_directories(fairmq-shmmonitor PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  )
  target_compile_definitions(fairmq-shmmonitor PUBLIC FAIRMQ_HAS_STD_FILESYSTEM=${FAIRMQ_HAS_STD_FILESYSTEM})

  add_executable(fairmq-uuid-gen tools/runUuidGenerator.cxx)
  target_link_libraries(fairmq-uuid-gen PUBLIC
    Boost::program_options
    Tools
  )


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
endif()

if(BUILD_SDK_COMMANDS)
  add_subdirectory(sdk/commands)
endif()

if(BUILD_SDK)
  add_subdirectory(sdk)
endif()

####################
# external plugins #
####################
if(BUILD_DDS_PLUGIN)
  add_subdirectory(plugins/DDS)
endif()
if(BUILD_PMIX_PLUGIN)
  add_subdirectory(plugins/PMIx)
endif()
