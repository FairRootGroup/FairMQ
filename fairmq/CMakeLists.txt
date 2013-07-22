INCLUDE_DIRECTORIES(
  ${BASE_INCLUDE_DIRECTORIES}
  ${CMAKE_SOURCE_DIR}/fairmq
  ${ZMQ_INCLUDE_DIR}
  ${ROOT_INCLUDE_DIR}
)

include_directories(${INCLUDE_DIRECTORIES})

SET(HEADERS
  "FairMQSampler.h"
  "FairMQBenchmarkSampler.h"
  #"FairMQStateMachine.h"
  "FairMQConfigurable.h"
  "FairMQDevice.h"
  "FairMQBuffer.h"
  "FairMQSamplerTask.h"
  "FairMQLogger.h"
  "FairMQContext.h"
  "FairMQMessage.h"
  "FairMQSocket.h"
  "FairMQBalancedStandaloneSplitter.h"
  "FairMQStandaloneMerger.h"
  "FairMQProcessor.h"
  "FairMQProcessorTask.h"
  "FairMQSink.h"
)

SET(SOURCES
  "FairMQSampler.cxx"
  "FairMQBenchmarkSampler.cxx"
  #"FairMQStateMachine.cxx"
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
)

set(LINK_DIRECTORIES
${ROOT_LIBRARY_DIR}
)

link_directories( ${LINK_DIRECTORIES})

SET(LINK_LIBRARIES
)


Set(SRCS ${SOURCES})
Set(LIBRARY_NAME FairMQ)
Set(LINKDEF)
Set(DEPENDENCIES   
  ${CMAKE_THREAD_LIBS_INIT}
  ${ZMQ_LIBRARY_SHARED}
  Base ParBase FairTools GeoBase 
  )

GENERATE_LIBRARY()

Set(Exe_Names bsampler buffer splitter merger sink)
Set(Exe_Source runBenchmarkSampler.cxx runBuffer.cxx runSplitter.cxx runMerger.cxx runSink.cxx)

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
