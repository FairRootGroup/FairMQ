 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/bsampler-sink.json ${CMAKE_BINARY_DIR}/bin/config/bsampler-sink.json)

add_subdirectory(logger)

add_subdirectory(examples/1-sampler-sink)
add_subdirectory(examples/2-sampler-processor-sink)
If(DDS_FOUND)
  add_subdirectory(examples/3-dds)
EndIf(DDS_FOUND)
add_subdirectory(examples/4-copypush)
add_subdirectory(examples/5-req-rep)
add_subdirectory(examples/6-multiple-channels)

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
    ${NANOMSG_INCLUDE_DIR}
  )
EndIf(NANOMSG_FOUND)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

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
  "devices/FairMQBuffer.cxx"
  "devices/FairMQProxy.cxx"
  "devices/FairMQSplitter.cxx"
  "devices/FairMQMerger.cxx"

  "options/FairProgOptions.cxx"
  "options/FairMQProgOptions.cxx"
  "options/FairMQParser.cxx"
)

If(PROTOBUF_FOUND)
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
  Set(DEPENDENCIES
    ${DEPENDENCIES}
    ${PROTOBUF_LIBRARY}
  )
Endif(PROTOBUF_FOUND)

If(NANOMSG_FOUND)
  Set(SRCS
    ${SRCS}
    "nanomsg/FairMQTransportFactoryNN.cxx"
    "nanomsg/FairMQMessageNN.cxx"
    "nanomsg/FairMQSocketNN.cxx"
    "nanomsg/FairMQPollerNN.cxx"
  )
  Set(DEPENDENCIES
    ${DEPENDENCIES}
    ${NANOMSG_LIBRARY_SHARED}
  )
EndIf(NANOMSG_FOUND)

# to copy src that are header-only files (e.g. c++ template) for FairRoot external installation
# manual install (globbing add not recommended)
Set(FAIRMQHEADERS
  devices/GenericSampler.h
  devices/GenericSampler.tpl
  devices/GenericProcessor.h
  devices/GenericFileSink.h
  devices/BaseDeserializationPolicy.h
  devices/BaseSerializationPolicy.h
  devices/BaseProcessorTaskPolicy.h
  devices/BaseSinkPolicy.h
  devices/BaseSourcePolicy.h
  tools/FairMQTools.h
)
Install(FILES ${FAIRMQHEADERS} DESTINATION include)

Set(DEPENDENCIES
  ${DEPENDENCIES}
  ${ZMQ_LIBRARY_SHARED}
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

Set(LIBRARY_NAME FairMQ)

GENERATE_LIBRARY()

Set(Exe_Names
  bsampler
  sink
  buffer
  splitter
  merger
  proxy
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

Set(Exe_Source
  run/runBenchmarkSampler.cxx
  run/runSink.cxx
  run/runBuffer.cxx
  run/runSplitter.cxx
  run/runMerger.cxx
  run/runProxy.cxx
)

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
  Set(EXE_NAME ${_name})
  Set(SRCS ${_src})
  Set(DEPENDENCIES FairMQ)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})
