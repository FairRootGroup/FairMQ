 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/2-sampler-processor-sink/ex2-sampler-processor-sink.json ${CMAKE_BINARY_DIR}/bin/config/ex2-sampler-processor-sink.json)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/2-sampler-processor-sink/startMQEx2.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/2-sampler-processor-sink/startMQEx2.sh)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/2-sampler-processor-sink/testMQEx2.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/2-sampler-processor-sink/testMQEx2.sh)

set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/2-sampler-processor-sink
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(SYSTEM_INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
  ${ZeroMQ_INCLUDE_DIR}
)

include_directories(${INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

link_directories(${LINK_DIRECTORIES})

set(SRCS
  "FairMQExample2Sampler.cxx"
  "FairMQExample2Processor.cxx"
  "FairMQExample2Sink.cxx"
)

set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

set(LIBRARY_NAME FairMQExample2)

GENERATE_LIBRARY()

set(Exe_Names
  ex2-sampler
  ex2-processor
  ex2-sink
)

set(Exe_Source
  runExample2Sampler.cxx
  runExample2Processor.cxx
  runExample2Sink.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(BIN_DESTINATION share/fairbase/examples/MQ/2-sampler-processor-sink/bin)
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/examples/MQ/2-sampler-processor-sink")

foreach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample2)
  GENERATE_EXECUTABLE()
endforeach(_file RANGE 0 ${_length})

add_test(NAME MQ.ex2-sampler-processor-sink COMMAND ${CMAKE_BINARY_DIR}/bin/examples/MQ/2-sampler-processor-sink/testMQEx2.sh)
set_tests_properties(MQ.ex2-sampler-processor-sink PROPERTIES TIMEOUT "30")
set_tests_properties(MQ.ex2-sampler-processor-sink PROPERTIES RUN_SERIAL true)
set_tests_properties(MQ.ex2-sampler-processor-sink PROPERTIES PASS_REGULAR_EXPRESSION "Received: ")

install(
  FILES ex2-sampler-processor-sink.json
  DESTINATION share/fairbase/examples/MQ/2-sampler-processor-sink/config/
)
