set(INCLUDE_DIRECTORIES
  ${BASE_INCLUDE_DIRECTORIES}
  ${CMAKE_SOURCE_DIR}/fairmq
  ${ZMQ_INCLUDE_DIR}
  ${NANOMSG_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${ROOT_INCLUDE_DIR}
)

include_directories(${INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${ROOT_LIBRARY_DIR}
  ${Boost_LIBRARY_DIRS}
)

link_directories(${LINK_DIRECTORIES})

Set(SRCS
  "FairMQSampler.cxx"
  "FairMQBenchmarkSampler.cxx"
  "FairMQStateMachine.cxx"
  "FairMQConfigurable.cxx"
  "FairMQBuffer.cxx"
  "FairMQSamplerTask.cxx"
  "FairMQLogger.cxx"
  "FairMQContext.cxx"
  "FairMQMessage.cxx"
  "FairMQTransportFactory.cxx"
  "FairMQTransportFactoryZMQ.cxx"
  "FairMQMessageZMQ.cxx"
  "FairMQMessageNN.cxx"
  "FairMQSocket.cxx"
  "FairMQSocketZMQ.cxx"
  "FairMQSocketNN.cxx"
  "FairMQSplitter.cxx"
  "FairMQMerger.cxx"
  "FairMQProcessor.cxx"
  "FairMQProcessorTask.cxx"
  "FairMQSink.cxx"
  "FairMQDevice.cxx"
  "FairMQProxy.cxx"
)

Set(DEPENDENCIES 
  ${CMAKE_THREAD_LIBS_INIT}
  ${ZMQ_LIBRARY_SHARED}
  ${NANOMSG_LIBRARY_SHARED}
  Base ParBase FairTools GeoBase boost_thread boost_timer boost_system
)

Set(LIBRARY_NAME FairMQ)

GENERATE_LIBRARY()

Set(Exe_Names bsampler buffer splitter merger sink proxy n_one_merger one_n_splitter)
Set(Exe_Source runBenchmarkSampler.cxx runBuffer.cxx runSplitter.cxx runMerger.cxx runSink.cxx runProxy.cxx runNToOneMerger.cxx runOneToNSplitter.cxx)

List(LENGTH Exe_Names _length)
Math(EXPR _length ${_length}-1)

ForEach(_file RANGE 0 ${_length})
  List(GET Exe_Names ${_file} _name)
  List(GET Exe_Source ${_file} _src)
  Set(EXE_NAME ${_name})
  Set(SRCS ${_src})
  Set(DEPENDENCIES FairMQ)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})
