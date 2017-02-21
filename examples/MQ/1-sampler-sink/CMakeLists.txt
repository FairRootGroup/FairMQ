 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/1-sampler-sink/ex1-sampler-sink.json
               ${CMAKE_BINARY_DIR}/bin/config/ex1-sampler-sink.json)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/1-sampler-sink/startMQEx1.sh.in
               ${CMAKE_BINARY_DIR}/bin/examples/MQ/1-sampler-sink/startMQEx1.sh)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/1-sampler-sink
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
  "FairMQExample1Sampler.cxx"
  "FairMQExample1Sink.cxx"
)

Set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

Set(LIBRARY_NAME FairMQExample1)

GENERATE_LIBRARY()

Set(Exe_Names
  ex1-sampler
  ex1-sink
)

Set(Exe_Source
  runExample1Sampler.cxx
  runExample1Sink.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(BIN_DESTINATION share/fairbase/examples/MQ/1-sampler-sink/bin)
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/examples/MQ/1-sampler-sink")

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample1)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})

Install(
  FILES ex1-sampler-sink.json
  DESTINATION share/fairbase/examples/MQ/1-sampler-sink/config/
)
