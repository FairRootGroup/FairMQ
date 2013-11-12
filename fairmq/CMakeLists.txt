include_directories(
  ${BASE_INCLUDE_DIRECTORIES}
  ${CMAKE_SOURCE_DIR}/fairmq
  ${ZMQ_INCLUDE_DIR}
  ${ROOT_INCLUDE_DIR}
)

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
  "FairMQSocket.cxx"
  "FairMQBalancedStandaloneSplitter.cxx"
  "FairMQStandaloneMerger.cxx"
  "FairMQProcessor.cxx"
  "FairMQProcessorTask.cxx"
  "FairMQSink.cxx"
  "FairMQDevice.cxx"
  "FairMQProxy.cxx"
)

Set(LINK_DIRECTORIES
  ${ROOT_LIBRARY_DIR}
  ${Boost_LIBRARY_DIRS}
)

link_directories(${LINK_DIRECTORIES})

Set(LIBRARY_NAME FairMQ)
Set(LINKDEF)
Set(DEPENDENCIES 
  ${CMAKE_THREAD_LIBS_INIT}
  ${ZMQ_LIBRARY_SHARED}
  Base ParBase FairTools GeoBase boost_thread boost_timer boost_system
)

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
