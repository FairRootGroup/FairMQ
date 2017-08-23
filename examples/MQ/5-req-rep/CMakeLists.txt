 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/5-req-rep/ex5-req-rep.json ${CMAKE_BINARY_DIR}/bin/config/ex5-req-rep.json)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/5-req-rep/startMQEx5.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/5-req-rep/startMQEx5.sh)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/5-req-rep/testMQEx5.sh.in ${CMAKE_BINARY_DIR}/bin/examples/MQ/5-req-rep/testMQEx5.sh)

set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/5-req-rep
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
  "FairMQExample5Client.cxx"
  "FairMQExample5Server.cxx"
)

set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

set(LIBRARY_NAME FairMQExample5)

GENERATE_LIBRARY()

set(Exe_Names
  ex5-client
  ex5-server
)

set(Exe_Source
  runExample5Client.cxx
  runExample5Server.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(BIN_DESTINATION share/fairbase/examples/MQ/5-req-rep/bin)
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/examples/MQ/5-req-rep")

foreach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample5)
  GENERATE_EXECUTABLE()
endforeach(_file RANGE 0 ${_length})

add_test(NAME MQ.ex5-req-rep COMMAND ${CMAKE_BINARY_DIR}/bin/examples/MQ/5-req-rep/testMQEx5.sh)
set_tests_properties(MQ.ex5-req-rep PROPERTIES TIMEOUT "30")
set_tests_properties(MQ.ex5-req-rep PROPERTIES RUN_SERIAL true)
set_tests_properties(MQ.ex5-req-rep PROPERTIES PASS_REGULAR_EXPRESSION "Received reply from server: ")

install(
  FILES ex5-req-rep.json
  DESTINATION share/fairbase/examples/MQ/5-req-rep/config/
)
