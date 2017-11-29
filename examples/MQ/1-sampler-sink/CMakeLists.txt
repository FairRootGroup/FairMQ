 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/1-sampler-sink/ex1-sampler-sink.json ${CMAKE_BINARY_DIR}/bin/config/ex1-sampler-sink.json)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/1-sampler-sink/startMQEx1.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/1-sampler-sink/startMQEx1.sh)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/1-sampler-sink/testMQEx1.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/1-sampler-sink/testMQEx1.sh)

set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/1-sampler-sink
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
  "FairMQExample1Sampler.cxx"
  "FairMQExample1Sink.cxx"
)

set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

set(LIBRARY_NAME FairMQExample1)

GENERATE_LIBRARY()

set(Exe_Names
  ex1-sampler
  ex1-sink
)

set(Exe_Source
  runExample1Sampler.cxx
  runExample1Sink.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(BIN_DESTINATION share/fairbase/examples/MQ/1-sampler-sink/bin)
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/examples/MQ/1-sampler-sink")

foreach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample1)
  GENERATE_EXECUTABLE()
endforeach(_file RANGE 0 ${_length})

add_test(NAME MQ.ex1-sampler-sink COMMAND ${CMAKE_BINARY_DIR}/bin/examples/MQ/1-sampler-sink/testMQEx1.sh)
set_tests_properties(MQ.ex1-sampler-sink PROPERTIES TIMEOUT "30")
set_tests_properties(MQ.ex1-sampler-sink PROPERTIES RUN_SERIAL true)
set_tests_properties(MQ.ex1-sampler-sink PROPERTIES PASS_REGULAR_EXPRESSION " Received: \"Hello\"")

install(
  FILES ex1-sampler-sink.json
  DESTINATION share/fairbase/examples/MQ/1-sampler-sink/config/
)
