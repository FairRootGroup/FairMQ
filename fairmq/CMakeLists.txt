 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/fairmq/run/startMQBenchmark.sh.in ${CMAKE_BINARY_DIR}/bin/startMQBenchmark.sh)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/run/benchmark.json ${CMAKE_BINARY_DIR}/bin/config/benchmark.json)

add_subdirectory(logger)
add_subdirectory(test)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/fairmq/logger
  ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  ${CMAKE_CURRENT_BINARY_DIR}
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
  ${ZMQ_INCLUDE_DIR}
)

If(DDS_FOUND)
  add_definitions(-DDDS_FOUND)
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${DDS_INCLUDE_DIR}
    )
EndIf(DDS_FOUND)

If(NANOMSG_FOUND)
  add_definitions(-DNANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  )
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${NANOMSG_INCLUDE_DIR}
  )
  If(MSGPACK_FOUND)
    add_definitions(-DMSGPACK_FOUND)
    Set(SYSTEM_INCLUDE_DIRECTORIES
      ${SYSTEM_INCLUDE_DIRECTORIES}
      ${MSGPACK_INCLUDE_DIR}
    )
  EndIf(MSGPACK_FOUND)
EndIf(NANOMSG_FOUND)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

If(DDS_FOUND)
  Set(LINK_DIRECTORIES
    ${LINK_DIRECTORIES}
    DDS_LIBRARY_DIR
    )
EndIf(DDS_FOUND)

Link_Directories(${LINK_DIRECTORIES})

Set(SRCS
  "zeromq/FairMQTransportFactoryZMQ.cxx"
  "zeromq/FairMQMessageZMQ.cxx"
  "zeromq/FairMQSocketZMQ.cxx"
  "zeromq/FairMQPollerZMQ.cxx"
  "zeromq/FairMQContextZMQ.cxx"

  "FairMQLogger.cxx"
  "FairMQConfigurable.cxx"
  "FairMQStateMachine.cxx"
  "FairMQTransportFactory.cxx"
  "FairMQMessage.cxx"
  "FairMQSocket.cxx"
  "FairMQChannel.cxx"
  "FairMQDevice.cxx"
  "FairMQPoller.cxx"

  "devices/FairMQBenchmarkSampler.cxx"
  "devices/FairMQSink.cxx"
  "devices/FairMQProxy.cxx"
  "devices/FairMQSplitter.cxx"
  "devices/FairMQMerger.cxx"

  "options/FairProgOptions.cxx"
  "options/FairMQProgOptions.cxx"
  "options/FairMQParser.cxx"
)

If(NANOMSG_FOUND)
  Set(SRCS
    ${SRCS}
    "nanomsg/FairMQTransportFactoryNN.cxx"
    "nanomsg/FairMQMessageNN.cxx"
    "nanomsg/FairMQSocketNN.cxx"
    "nanomsg/FairMQPollerNN.cxx"
  )
EndIf(NANOMSG_FOUND)


# to copy src that are header-only files (e.g. c++ template) for FairRoot external installation
# manual install (globbing add not recommended)
Set(FAIRMQHEADERS
  FairMQParts.h
  devices/GenericSampler.h
  devices/GenericSampler.tpl
  devices/GenericProcessor.h
  devices/GenericFileSink.h
  devices/BaseDeserializationPolicy.h
  devices/BaseSerializationPolicy.h
  devices/BaseProcessorTaskPolicy.h
  devices/BaseSinkPolicy.h
  devices/BaseSourcePolicy.h
  options/FairProgOptionsHelper.h
  tools/FairMQTools.h
  tools/FairMQDDSTools.h
  tools/runSimpleMQStateMachine.h
)
Install(FILES ${FAIRMQHEADERS} DESTINATION include)

Set(DEPENDENCIES
  ${DEPENDENCIES}
  ${ZMQ_LIBRARY_SHARED}
  ${Boost_THREAD_LIBRARY}
  fairmq_logger
  ${Boost_TIMER_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_RANDOM_LIBRARY}
  ${Boost_CHRONO_LIBRARY}
  ${Boost_REGEX_LIBRARY}
)

If(NANOMSG_FOUND)
  Set(DEPENDENCIES
    ${DEPENDENCIES}
    ${NANOMSG_LIBRARY_SHARED}
    # msgpackc # currently header only
  )
EndIf(NANOMSG_FOUND)

If(DDS_FOUND)
  Set(DEPENDENCIES
    ${DEPENDENCIES}
    ${DDS_INTERCOM_LIBRARY_SHARED}
    ${DDS_PROTOCOL_LIBRARY_SHARED} # also link the two DDS dependency libraries to avoid linking issues on some osx systems
    ${DDS_USER_DEFAULTS_LIBRARY_SHARED}
    )
EndIf(DDS_FOUND)

Set(LIBRARY_NAME FairMQ)

GENERATE_LIBRARY()

Set(Exe_Names
  bsampler
  sink
  splitter
  merger
  proxy
)

If(DDS_FOUND)
  Set(Exe_Names
    ${Exe_Names}
    fairmq-dds-command-ui
    )
EndIf(DDS_FOUND)

Set(Exe_Source
  run/runBenchmarkSampler.cxx
  run/runSink.cxx
  run/runSplitter.cxx
  run/runMerger.cxx
  run/runProxy.cxx
)

If(DDS_FOUND)
  Set(Exe_Source
    ${Exe_Source}
    run/runDDSCommandUI.cxx
    )
EndIf(DDS_FOUND)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  Set(EXE_NAME ${_name})
  Set(SRCS ${_src})
  Set(DEPENDENCIES FairMQ)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})
