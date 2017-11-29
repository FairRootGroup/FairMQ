 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/6-multiple-channels/ex6-multiple-channels.json ${CMAKE_BINARY_DIR}/bin/config/ex6-multiple-channels.json)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/6-multiple-channels/startMQEx6.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/6-multiple-channels/startMQEx6.sh)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/6-multiple-channels/testMQEx6.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/6-multiple-channels/testMQEx6.sh)

set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/6-multiple-channels
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
  "FairMQExample6Sampler.cxx"
  "FairMQExample6Sink.cxx"
  "FairMQExample6Broadcaster.cxx"
)

set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

set(LIBRARY_NAME FairMQExample6)

GENERATE_LIBRARY()

set(Exe_Names
  ex6-sampler
  ex6-sink
  ex6-broadcaster
)

set(Exe_Source
  runExample6Sampler.cxx
  runExample6Sink.cxx
  runExample6Broadcaster.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(BIN_DESTINATION share/fairbase/examples/MQ/6-multiple-channels/bin)
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/examples/MQ/6-multiple-channels")

foreach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample6)
  GENERATE_EXECUTABLE()
endforeach(_file RANGE 0 ${_length})

add_test(NAME MQ.ex6-multiple-channels COMMAND ${CMAKE_BINARY_DIR}/bin/examples/MQ/6-multiple-channels/testMQEx6.sh)
set_tests_properties(MQ.ex6-multiple-channels PROPERTIES TIMEOUT "30")
set_tests_properties(MQ.ex6-multiple-channels PROPERTIES RUN_SERIAL true)
set_tests_properties(MQ.ex6-multiple-channels PROPERTIES PASS_REGULAR_EXPRESSION "Received messages from both sources.")

install(
  FILES ex6-multiple-channels.json
  DESTINATION share/fairbase/examples/MQ/6-multiple-channels/config/
)
