 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/bsampler-sink.json ${CMAKE_BINARY_DIR}/bin/config/example-bsampler-sink.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/1-sampler-sink/sampler-sink.json ${CMAKE_BINARY_DIR}/bin/config/ex1-sampler-sink.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/2-sampler-processor-sink/sampler-processor-sink.json ${CMAKE_BINARY_DIR}/bin/config/ex2-sampler-processor-sink.json)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/fairmq/examples/1-sampler-sink
  ${CMAKE_SOURCE_DIR}/fairmq/examples/2-sampler-processor-sink
  ${CMAKE_SOURCE_DIR}/fairmq/examples/req-rep
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
)

If(PROTOBUF_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    # # following directory is only for protobuf tests and is not essential part of FairMQ
    #${CMAKE_SOURCE_DIR}/fairmq/prototest
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

  "examples/req-rep/FairMQExampleClient.cxx"
  "examples/req-rep/FairMQExampleServer.cxx"
)

if(PROTOBUF_FOUND)
  # following source files are only for protobuf tests and are not essential part of FairMQ
  # set(SRCS
  #   ${SRCS}
  #   "prototest/payload.pb.cc"
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
  boost_thread boost_timer boost_system boost_filesystem boost_program_options boost_random boost_chrono boost_exception
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
  example-client
  example-server
)

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
  examples/req-rep/runExampleClient.cxx
  examples/req-rep/runExampleServer.cxx
)

# following source files are only for protobuf tests and are not essential part of FairMQ
# if(PROTOBUF_FOUND)
#   set(Exe_Source
#       ${Exe_Source}
#       run/runBinSampler.cxx
#       run/runProtoSampler.cxx
#       run/runBinSink.cxx
#       run/runProtoSink.cxx
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
