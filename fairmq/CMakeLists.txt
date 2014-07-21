 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             # 
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #  
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################
set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${Boost_INCLUDE_DIR}
)

if(PROTOBUF_FOUND)
  set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${PROTOBUF_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/fairmq/prototest
  )
endif(PROTOBUF_FOUND)

if(NANOMSG_FOUND)
  set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${NANOMSG_LIBRARY_SHARED}
    ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  )
else(NANOMSG_FOUND)
  set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${ZMQ_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  )
endif(NANOMSG_FOUND)

include_directories(${INCLUDE_DIRECTORIES})

set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

link_directories(${LINK_DIRECTORIES})

set(SRCS
  "FairMQLogger.cxx"
  "FairMQConfigurable.cxx"
  "FairMQStateMachine.cxx"
  "FairMQTransportFactory.cxx"
  "FairMQMessage.cxx"
  "FairMQSocket.cxx"
  "FairMQDevice.cxx"
  "devices/FairMQBenchmarkSampler.cxx"
  "devices/FairMQSink.cxx"
  "devices/FairMQBuffer.cxx"
  "devices/FairMQProxy.cxx"
  "devices/FairMQSplitter.cxx"
  "devices/FairMQMerger.cxx"
  "FairMQPoller.cxx"
)

if(PROTOBUF_FOUND)
  set(SRCS
    ${SRCS}
    "prototest/payload.pb.cc"
    "prototest/FairMQProtoSampler.cxx"
    "prototest/FairMQBinSampler.cxx"
    "prototest/FairMQBinSink.cxx"
    "prototest/FairMQProtoSink.cxx"
  )
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

set(DEPENDENCIES
  ${DEPENDENCIES}
  boost_thread boost_timer boost_system
)

set(LIBRARY_NAME FairMQ)

GENERATE_LIBRARY()

set(Exe_Names
  bsampler
  buffer
  splitter
  merger
  sink
  proxy)

if(PROTOBUF_FOUND)
  set(Exe_Names
      ${Exe_Names}
      binsampler
      protosampler
      binsink
      protosink
      )
endif(PROTOBUF_FOUND)

set(Exe_Source 
  run/runBenchmarkSampler.cxx
  run/runBuffer.cxx
  run/runSplitter.cxx
  run/runMerger.cxx
  run/runSink.cxx
  run/runProxy.cxx
)

if(PROTOBUF_FOUND)
  set(Exe_Source
      ${Exe_Source}
      run/runBinSampler.cxx
      run/runProtoSampler.cxx
      run/runBinSink.cxx
      run/runProtoSink.cxx
      )
endif(PROTOBUF_FOUND)

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
