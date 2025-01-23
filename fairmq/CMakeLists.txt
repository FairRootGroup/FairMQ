################################################################################
# Copyright (C) 2012-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

if(BUILD_FAIRMQ)

  if(BUILD_TIDY_TOOL)
    include(FairMQTidy)
  endif()

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

  ##########################
  # libFairMQ header files #
  ##########################
  set(FAIRMQ_PUBLIC_HEADER_FILES
    Channel.h
    Device.h
    DeviceRunner.h
    Error.h
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
    FwdDecls.h
    JSONParser.h
    MemoryResourceTools.h
    MemoryResources.h
    Message.h
    Parts.h
    Plugin.h
    PluginManager.h
    PluginServices.h
    Poller.h
    ProgOptions.h
    ProgOptionsFwd.h
    Properties.h
    PropertyOutput.h
    Socket.h
    StateMachine.h
    States.h
    StateQueue.h
    SuboptParser.h
    Tools.h
    TransportFactory.h
    Transports.h
    TransportEnum.h
    UnmanagedRegion.h
    options/FairMQProgOptions.h
    runDevice.h
    runFairMQDevice.h
    shmem/Common.h
    shmem/Monitor.h
    shmem/Segment.h
    shmem/UnmanagedRegion.h
    tools/Compiler.h
    tools/CppSTL.h
    tools/Exceptions.h
    tools/IO.h
    tools/InstanceLimit.h
    tools/Network.h
    tools/Process.h
    tools/RateLimit.h
    tools/Semaphore.h
    tools/Strings.h
    tools/Unique.h
    tools/Version.h
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
    shmem/UnmanagedRegionImpl.h
    shmem/Socket.h
    shmem/TransportFactory.h
    shmem/Manager.h
    zeromq/Common.h
    zeromq/Context.h
    zeromq/Message.h
    zeromq/Poller.h
    zeromq/UnmanagedRegion.h
    zeromq/Socket.h
    zeromq/TransportFactory.h
    zeromq/ZMsg.h
  )

  ##########################
  # libFairMQ source files #
  ##########################
  set(FAIRMQ_SOURCE_FILES
    Channel.cxx
    Device.cxx
    DeviceRunner.cxx
    EventManager.cxx
    JSONParser.cxx
    MemoryResources.cxx
    Plugin.cxx
    PluginManager.cxx
    PluginServices.cxx
    ProgOptions.cxx
    Properties.cxx
    StateMachine.cxx
    States.cxx
    SuboptParser.cxx
    TransportFactory.cxx
    plugins/config/Config.cxx
    plugins/control/Control.cxx
    shmem/Common.cxx
    shmem/Manager.cxx
    shmem/Monitor.cxx
    tools/Network.cxx
    tools/Process.cxx
    tools/Semaphore.cxx
    tools/Unique.cxx
  )


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
  target_compile_definitions(${target} PUBLIC
    BOOST_ERROR_CODE_HEADER_ONLY
    BOOST_ASIO_HAS_HAS_STD_CHRONO
  )
  if(FAIRMQ_DEBUG_MODE)
    target_compile_definitions(${target} PUBLIC FAIRMQ_DEBUG_MODE)
  endif()
  target_compile_definitions(${target} PUBLIC
    FAIRMQ_HAS_STD_FILESYSTEM=${FAIRMQ_HAS_STD_FILESYSTEM}
    FAIRMQ_HAS_STD_PMR=${FAIRMQ_HAS_STD_PMR}
  )
  if(DEFINED FAIRMQ_CHANNEL_DEFAULT_AUTOBIND)
    # translate CMake boolean (TRUE, FALSE, 0, 1, OFF, ON) into C++ boolean literal (true, false)
    if(FAIRMQ_CHANNEL_DEFAULT_AUTOBIND)
      set(value "true")
    else()
      set(value "false")
    endif()
    target_compile_definitions(${target} PUBLIC FAIRMQ_CHANNEL_DEFAULT_AUTOBIND=${value})
  endif()


  #######################
  # include directories #
  #######################
  target_include_directories(${target}
    PUBLIC # consumers inherit public include directories
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
    $<INSTALL_INTERFACE:${PROJECT_INSTALL_INCDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  ##################
  # link libraries #
  ##################
  target_link_libraries(${target}
    INTERFACE # only consumers link against interface dependencies
    Boost::container

    PUBLIC # libFairMQ AND consumers of libFairMQ link aginst public dependencies
    Threads::Threads
    dl
    $<$<PLATFORM_ID:Linux>:rt>
    Boost::boost
    Boost::program_options
    Boost::filesystem # still needed for Boost.DLL
    Boost::regex
    FairLogger::FairLogger

    PRIVATE # only libFairMQ links against private dependencies
    libzmq
    PicoSHA2
  )
  set_target_properties(${target} PROPERTIES
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME ${PROJECT_NAME_LOWER}
  )
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET ${target})
  endif()


  ###############
  # executables #
  ###############
  add_executable(fairmq-bsampler devices/runBenchmarkSampler.cxx)
  target_link_libraries(fairmq-bsampler FairMQ)
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET fairmq-bsampler)
  endif()

  add_executable(fairmq-merger devices/runMerger.cxx)
  target_link_libraries(fairmq-merger FairMQ)
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET fairmq-merger)
  endif()

  add_executable(fairmq-multiplier devices/runMultiplier.cxx)
  target_link_libraries(fairmq-multiplier FairMQ)
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET fairmq-multiplier)
  endif()

  add_executable(fairmq-proxy devices/runProxy.cxx)
  target_link_libraries(fairmq-proxy FairMQ)
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET fairmq-proxy)
  endif()

  add_executable(fairmq-sink devices/runSink.cxx)
  target_link_libraries(fairmq-sink FairMQ)
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET fairmq-sink)
  endif()

  add_executable(fairmq-splitter devices/runSplitter.cxx)
  target_link_libraries(fairmq-splitter FairMQ)
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET fairmq-splitter)
  endif()

  add_executable(fairmq-shmmonitor shmem/Common.cxx shmem/Monitor.cxx shmem/Monitor.h shmem/runMonitor.cxx)
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
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET fairmq-shmmonitor)
  endif()

  add_executable(fairmq-uuid-gen tools/runUuidGenerator.cxx)
  target_link_libraries(fairmq-uuid-gen PUBLIC
    Boost::program_options
    FairMQ
  )
  if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
    fairmq_target_tidy(TARGET fairmq-uuid-gen)
  endif()


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
