set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${Boost_INCLUDE_DIR}
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
  "FairMQBenchmarkSampler.cxx"
  "FairMQSink.cxx"
  "FairMQBuffer.cxx"
  "FairMQProxy.cxx"
  "FairMQSplitter.cxx"
  "FairMQMerger.cxx"
  "FairMQPoller.cxx"
)

if(NANOMSG_FOUND)
  set(SRCS
    ${SRCS}
    "nanomsg/FairMQTransportFactoryNN.cxx"
    "nanomsg/FairMQMessageNN.cxx"
    "nanomsg/FairMQSocketNN.cxx"
    "nanomsg/FairMQPollerNN.cxx"
  )
  set(DEPENDENCIES
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
    ${ZMQ_LIBRARY_SHARED}
  )
endif(NANOMSG_FOUND)

set(DEPENDENCIES
  ${DEPENDENCIES}
  ${CMAKE_THREAD_LIBS_INIT}
  boost_thread boost_timer boost_system
)

set(LIBRARY_NAME FairMQ)

GENERATE_LIBRARY()

set(Exe_Names bsampler buffer splitter merger sink proxy n_one_merger one_n_splitter)

if(NANOMSG_FOUND)
  set(Exe_Source 
    nanomsg/runBenchmarkSampler.cxx
    nanomsg/runBuffer.cxx
    nanomsg/runSplitter.cxx
    nanomsg/runMerger.cxx
    nanomsg/runSink.cxx
    nanomsg/runProxy.cxx
    nanomsg/runNToOneMerger.cxx
    nanomsg/runOneToNSplitter.cxx
  )
else(NANOMSG_FOUND)
  set(Exe_Source 
    zeromq/runBenchmarkSampler.cxx
    zeromq/runBuffer.cxx
    zeromq/runSplitter.cxx
    zeromq/runMerger.cxx
    zeromq/runSink.cxx
    zeromq/runProxy.cxx
    zeromq/runNToOneMerger.cxx
    zeromq/runOneToNSplitter.cxx
  )
endif(NANOMSG_FOUND)

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
