 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/6-multiple-channels/ex6-multiple-channels.json ${CMAKE_BINARY_DIR}/bin/config/ex6-multiple-channels.json)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/6-multiple-channels/startMQEx6.sh.in  ${CMAKE_BINARY_DIR}/bin/startMQEx6.sh)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/6-multiple-channels
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
  "FairMQExample6Sampler.cxx"
  "FairMQExample6Sink.cxx"
  "FairMQExample6Broadcaster.cxx"
)

Set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

Set(LIBRARY_NAME FairMQExample6)

GENERATE_LIBRARY()

Set(Exe_Names
  ex6-sampler
  ex6-sink
  ex6-broadcaster
)

Set(Exe_Source
  runExample6Sampler.cxx
  runExample6Sink.cxx
  runExample6Broadcaster.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample6)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})

Install(
  FILES ex6-multiple-channels.json
  DESTINATION share/fairbase/examples/MQ/6-multiple-channels/config/
)
