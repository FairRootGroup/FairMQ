 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/bsampler-sink.json ${CMAKE_BINARY_DIR}/bin/config/bsampler-sink.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/1-sampler-sink/ex1-sampler-sink.json ${CMAKE_BINARY_DIR}/bin/config/ex1-sampler-sink.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/2-sampler-processor-sink/ex2-sampler-processor-sink.json ${CMAKE_BINARY_DIR}/bin/config/ex2-sampler-processor-sink.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/3-dds/ex3-devices.json ${CMAKE_BINARY_DIR}/bin/config/ex3-devices.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/3-dds/ex3-dds-topology.xml ${CMAKE_BINARY_DIR}/bin/config/ex3-dds-topology.xml)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/3-dds/ex3-dds-hosts.cfg ${CMAKE_BINARY_DIR}/bin/config/ex3-dds-hosts.cfg COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/4-copypush/ex4-copypush.json ${CMAKE_BINARY_DIR}/bin/config/ex4-copypush.json)

add_subdirectory(logger)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/fairmq/logger
  ${CMAKE_SOURCE_DIR}/fairmq/examples/1-sampler-sink
  ${CMAKE_SOURCE_DIR}/fairmq/examples/2-sampler-processor-sink
  ${CMAKE_SOURCE_DIR}/fairmq/examples/4-copypush
  ${CMAKE_SOURCE_DIR}/fairmq/examples/5-req-rep
  ${CMAKE_CURRENT_BINARY_DIR}
)

if(DDS_FOUND)
  add_definitions(-DENABLE_DDS)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/examples/3-dds
  )
endif(DDS_FOUND)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
)

If(DDS_FOUND)
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${DDS_INCLUDE_DIR}
    )
EndIf(DDS_FOUND)

If(PROTOBUF_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    # following directory is only for protobuf tests and is not essential part of FairMQ
    # ${CMAKE_SOURCE_DIR}/fairmq/prototest
  )
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${PROTOBUF_INCLUDE_DIR}
  )
EndIf(PROTOBUF_FOUND)

If(NANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  )
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${NANOMSG_LIBRARY_SHARED}
  )
Else(NANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  )
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${ZMQ_INCLUDE_DIR}
  )
EndIf(NANOMSG_FOUND)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

if(DDS_FOUND)
  set(LINK_DIRECTORIES
    ${LINK_DIRECTORIES}
    ${DDS_LIBRARY_DIR}
  )
endif(DDS_FOUND)

Link_Directories(${LINK_DIRECTORIES})

set(SRCS
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
  "devices/FairMQBuffer.cxx"
  "devices/FairMQProxy.cxx"
  "devices/FairMQSplitter.cxx"
  "devices/FairMQMerger.cxx"

  "options/FairProgOptions.cxx"
  "options/FairMQProgOptions.cxx"
  "options/FairMQParser.cxx"

  "examples/1-sampler-sink/FairMQExample1Sampler.cxx"
  "examples/1-sampler-sink/FairMQExample1Sink.cxx"

  "examples/2-sampler-processor-sink/FairMQExample2Sampler.cxx"
  "examples/2-sampler-processor-sink/FairMQExample2Processor.cxx"
  "examples/2-sampler-processor-sink/FairMQExample2Sink.cxx"

  "examples/4-copypush/FairMQExample4Sampler.cxx"
  "examples/4-copypush/FairMQExample4Sink.cxx"

  "examples/5-req-rep/FairMQExample5Client.cxx"
  "examples/5-req-rep/FairMQExample5Server.cxx"
)

if(DDS_FOUND)
  set(SRCS
    ${SRCS}
    "examples/3-dds/FairMQExample3Sampler.cxx"
    "examples/3-dds/FairMQExample3Processor.cxx"
    "examples/3-dds/FairMQExample3Sink.cxx"
  )
  set(DEPENDENCIES
    ${DEPENDENCIES}
    dds-key-value-lib
  )
endif(DDS_FOUND)

if(PROTOBUF_FOUND)
  # following source files are only for protobuf tests and are not essential part of FairMQ
  # add_custom_command(
  #   OUTPUT
  #     ${CMAKE_CURRENT_BINARY_DIR}/payload.pb.h
  #     ${CMAKE_CURRENT_BINARY_DIR}/payload.pb.cc
  #   COMMAND
  #     ${SIMPATH}/bin/protoc -I=. --cpp_out=${CMAKE_CURRENT_BINARY_DIR} payload.proto
  #   WORKING_DIRECTORY
  #     ${CMAKE_SOURCE_DIR}/fairmq/prototest
  #   )
  # set(SRCS
  #   ${SRCS}
  #   ${CMAKE_CURRENT_BINARY_DIR}/payload.pb.cc
  #   "prototest/FairMQProtoSampler.cxx"
  #   "prototest/FairMQBinSampler.cxx"
  #   "prototest/FairMQBinSink.cxx"
  #   "prototest/FairMQProtoSink.cxx"
  # )
  set(DEPENDENCIES
    ${DEPENDENCIES}
    ${PROTOBUF_LIBRARY}
  )
endif(PROTOBUF_FOUND)

if(NANOMSG_FOUND)
  set(SRCS
    ${SRCS}
    "nanomsg/FairMQTransportFactoryNN.cxx"
    "nanomsg/FairMQMessageNN.cxx"
    "nanomsg/FairMQSocketNN.cxx"
    "nanomsg/FairMQPollerNN.cxx"
  )
  set(DEPENDENCIES
    ${DEPENDENCIES}
    ${NANOMSG_LIBRARY_SHARED}
  )
else(NANOMSG_FOUND)
  set(SRCS
    ${SRCS}
    "zeromq/FairMQTransportFactoryZMQ.cxx"
    "zeromq/FairMQMessageZMQ.cxx"
    "zeromq/FairMQSocketZMQ.cxx"
    "zeromq/FairMQPollerZMQ.cxx"
    "zeromq/FairMQContextZMQ.cxx"
  )
  set(DEPENDENCIES
    ${DEPENDENCIES}
    ${ZMQ_LIBRARY_SHARED}
  )
endif(NANOMSG_FOUND)

# to copy src that are header-only files (e.g. c++ template) for FairRoot external installation
# manual install (globbing add not recommended)
Set(FAIRMQHEADERS
  devices/GenericSampler.h
  devices/GenericSampler.tpl
  devices/GenericProcessor.h
  devices/GenericFileSink.h
  tools/FairMQTools.h
)
install(FILES ${FAIRMQHEADERS} DESTINATION include)

set(DEPENDENCIES
  ${DEPENDENCIES}
  boost_thread
  fairmq_logger
  boost_timer
  boost_system
  boost_filesystem
  boost_program_options
  boost_random
  boost_chrono
  boost_exception
)

set(LIBRARY_NAME FairMQ)

GENERATE_LIBRARY()

set(Exe_Names
  bsampler
  sink
  buffer
  splitter
  merger
  proxy
  ex1-sampler
  ex1-sink
  ex2-sampler
  ex2-processor
  ex2-sink
  ex4-sampler
  ex4-sink
  ex5-client
  ex5-server
)

if(DDS_FOUND)
  set(Exe_Names
    ${Exe_Names}
    ex3-sampler-dds
    ex3-processor-dds
    ex3-sink-dds
  )
endif(DDS_FOUND)

# following executables are only for protobuf tests and are not essential part of FairMQ
# if(PROTOBUF_FOUND)
#   set(Exe_Names
#       ${Exe_Names}
#       binsampler
#       protosampler
#       binsink
#       protosink
#       )
# endif(PROTOBUF_FOUND)

set(Exe_Source 
  run/runBenchmarkSampler.cxx
  run/runSink.cxx
  run/runBuffer.cxx
  run/runSplitter.cxx
  run/runMerger.cxx
  run/runProxy.cxx
  examples/1-sampler-sink/runExample1Sampler.cxx
  examples/1-sampler-sink/runExample1Sink.cxx
  examples/2-sampler-processor-sink/runExample2Sampler.cxx
  examples/2-sampler-processor-sink/runExample2Processor.cxx
  examples/2-sampler-processor-sink/runExample2Sink.cxx
  examples/4-copypush/runExample4Sampler.cxx
  examples/4-copypush/runExample4Sink.cxx
  examples/5-req-rep/runExample5Client.cxx
  examples/5-req-rep/runExample5Server.cxx
)

if(DDS_FOUND)
  set(Exe_Source
    ${Exe_Source}
    examples/3-dds/runExample3Sampler.cxx
    examples/3-dds/runExample3Processor.cxx
    examples/3-dds/runExample3Sink.cxx
  )
endif(DDS_FOUND)

# following source files are only for protobuf tests and are not essential part of FairMQ
# if(PROTOBUF_FOUND)
#   set(Exe_Source
#       ${Exe_Source}
#       prototest/runBinSampler.cxx
#       prototest/runProtoSampler.cxx
#       prototest/runBinSink.cxx
#       prototest/runProtoSink.cxx
#       )
# endif(PROTOBUF_FOUND)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQ)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})
