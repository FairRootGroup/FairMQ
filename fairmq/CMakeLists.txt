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
  ${CMAKE_THREAD_LIBS_INIT}
  ${ZMQ_LIBRARY_SHARED}
  ${ROOT_LIBRARIES}
  Base ParBase FairTools GeoBase 
)


ADD_LIBRARY(FairMQ SHARED ${HEADERS} ${SOURCES})
SET_TARGET_PROPERTIES(FairMQ PROPERTIES  ${FAIRROOT_LIBRARY_PROPERTIES})
TARGET_LINK_LIBRARIES(FairMQ ${LINK_LIBRARIES})

#Set(LIBRARY_NAME FairMQ)

#GENERATE_LIBRARY()

ADD_LIBRARY(FairMQStatic STATIC ${HEADERS} ${SOURCES})
#SET_TARGET_PROPERTIES(FairMQStatic PROPERTIES OUTPUT_NAME FairMQ)
TARGET_LINK_LIBRARIES(FairMQStatic ${LINK_LIBRARIES})

ADD_EXECUTABLE(RunBenchmarkSampler runBenchmarkSampler.cxx)
SET_TARGET_PROPERTIES(RunBenchmarkSampler PROPERTIES OUTPUT_NAME bsampler)
TARGET_LINK_LIBRARIES(RunBenchmarkSampler FairMQ)

ADD_EXECUTABLE(RunBuffer runBuffer.cxx)
SET_TARGET_PROPERTIES(RunBuffer PROPERTIES OUTPUT_NAME buffer)
TARGET_LINK_LIBRARIES(RunBuffer FairMQ)

ADD_EXECUTABLE(RunSplitter runSplitter.cxx)
SET_TARGET_PROPERTIES(RunSplitter PROPERTIES OUTPUT_NAME splitter)
TARGET_LINK_LIBRARIES(RunSplitter FairMQ)

ADD_EXECUTABLE(RunMerger runMerger.cxx)
SET_TARGET_PROPERTIES(RunMerger PROPERTIES OUTPUT_NAME merger)
TARGET_LINK_LIBRARIES(RunMerger FairMQ)

ADD_EXECUTABLE(RunSink runSink.cxx)
SET_TARGET_PROPERTIES(RunSink PROPERTIES OUTPUT_NAME sink)
TARGET_LINK_LIBRARIES(RunSink FairMQ)


