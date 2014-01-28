set(INCLUDE_DIRECTORIES
  ${BASE_INCLUDE_DIRECTORIES}
  ${CMAKE_SOURCE_DIR}/fairmq
  ${Boost_INCLUDE_DIR}
  ${ROOT_INCLUDE_DIR}
)

if(NANOMSG_FOUND)
  set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${NANOMSG_LIBRARY_SHARED}
  )
else(NANOMSG_FOUND)
  set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${ZMQ_INCLUDE_DIR}
  )
endif(NANOMSG_FOUND)

include_directories(${INCLUDE_DIRECTORIES})

set(LINK_DIRECTORIES
  ${ROOT_LIBRARY_DIR}
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
  "FairMQSampler.cxx"
  "FairMQBenchmarkSampler.cxx"
  "FairMQProcessor.cxx"
  "FairMQSink.cxx"
  "FairMQBuffer.cxx"
  "FairMQProxy.cxx"
  "FairMQSplitter.cxx"
  "FairMQMerger.cxx"
  "FairMQPoller.cxx"
  "FairMQSamplerTask.cxx"
  "FairMQProcessorTask.cxx"
)

if(NANOMSG_FOUND)
  set(SRCS
    ${SRCS}
    "FairMQTransportFactoryNN.cxx"
    "FairMQMessageNN.cxx"
    "FairMQSocketNN.cxx"
    "FairMQPollerNN.cxx"
  )
  set(DEPENDENCIES
    ${NANOMSG_LIBRARY_SHARED}
  )
else(NANOMSG_FOUND)
  set(SRCS
    ${SRCS}
    "FairMQTransportFactoryZMQ.cxx"
    "FairMQMessageZMQ.cxx"
    "FairMQSocketZMQ.cxx"
    "FairMQPollerZMQ.cxx"
    "FairMQContextZMQ.cxx"
  )
  set(DEPENDENCIES
    ${ZMQ_LIBRARY_SHARED}
  )
endif(NANOMSG_FOUND)

set(DEPENDENCIES
  ${DEPENDENCIES}
  ${CMAKE_THREAD_LIBS_INIT}
  Base ParBase FairTools GeoBase boost_thread boost_timer boost_system
)

set(LIBRARY_NAME FairMQ)

GENERATE_LIBRARY()

set(Exe_Names bsampler buffer splitter merger sink proxy n_one_merger one_n_splitter)
set(Exe_Source runBenchmarkSampler.cxx runBuffer.cxx runSplitter.cxx runMerger.cxx runSink.cxx runProxy.cxx runNToOneMerger.cxx runOneToNSplitter.cxx)

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
