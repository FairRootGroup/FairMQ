 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/4-copypush/ex4-copypush.json 
               ${CMAKE_BINARY_DIR}/bin/config/ex4-copypush.json)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/4-copypush/startMQEx4.sh.in  
               ${CMAKE_BINARY_DIR}/bin/examples/MQ/4-copypush/startMQEx4.sh)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/4-copypush
  ${CMAKE_CURRENT_BINARY_DIR}
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
  ${ZMQ_INCLUDE_DIR}
)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

Link_Directories(${LINK_DIRECTORIES})

Set(SRCS
  "FairMQExample4Sampler.cxx"
  "FairMQExample4Sink.cxx"
)

Set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

Set(LIBRARY_NAME FairMQExample4)

GENERATE_LIBRARY()

Set(Exe_Names
  ex4-sampler
  ex4-sink
)

Set(Exe_Source
  runExample4Sampler.cxx
  runExample4Sink.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(BIN_DESTINATION share/fairbase/examples/MQ/4-copypush/bin)
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/examples/MQ/4-copypush")

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  Set(EXE_NAME ${_name})
  Set(SRCS ${_src})
  Set(DEPENDENCIES FairMQExample4)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})

Install(
  FILES ex4-copypush.json
  DESTINATION share/fairbase/examples/MQ/4-copypush/config/
)
