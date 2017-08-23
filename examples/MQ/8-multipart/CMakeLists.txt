 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/8-multipart/ex8-multipart.json ${CMAKE_BINARY_DIR}/bin/config/ex8-multipart.json)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/8-multipart/startMQEx8.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/8-multipart/startMQEx8.sh)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/8-multipart/testMQEx8.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/8-multipart/testMQEx8.sh)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/8-multipart
  ${CMAKE_CURRENT_BINARY_DIR}
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
  ${ZeroMQ_INCLUDE_DIR}
)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

Link_Directories(${LINK_DIRECTORIES})

Set(SRCS
  "FairMQExample8Sampler.cxx"
  "FairMQExample8Sink.cxx"
)

Set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

Set(LIBRARY_NAME FairMQExample8)

GENERATE_LIBRARY()

Set(Exe_Names
  ex8-sampler
  ex8-sink
)

Set(Exe_Source
  runExample8Sampler.cxx
  runExample8Sink.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(BIN_DESTINATION share/fairbase/examples/MQ/8-multipart/bin)
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/examples/MQ/8-multipart")

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  Set(EXE_NAME ${_name})
  Set(SRCS ${_src})
  Set(DEPENDENCIES FairMQExample8)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})

add_test(NAME MQ.ex8-multipart COMMAND ${CMAKE_BINARY_DIR}/bin/examples/MQ/8-multipart/testMQEx8.sh)
set_tests_properties(MQ.ex8-multipart PROPERTIES TIMEOUT "30")
set_tests_properties(MQ.ex8-multipart PROPERTIES RUN_SERIAL true)
set_tests_properties(MQ.ex8-multipart PROPERTIES PASS_REGULAR_EXPRESSION "Received message with 2 parts")

Install(
  FILES ex8-multipart.json
  DESTINATION share/fairbase/examples/MQ/8-multipart/config/
)
