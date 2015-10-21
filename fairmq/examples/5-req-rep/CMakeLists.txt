 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/fairmq/examples/5-req-rep
  ${CMAKE_CURRENT_BINARY_DIR}
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
)

If(NANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  )
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${ZMQ_INCLUDE_DIR}
  )
Else(NANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  )
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${ZMQ_INCLUDE_DIR}
  )
EndIf(NANOMSG_FOUND)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

Link_Directories(${LINK_DIRECTORIES})

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

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample5)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})
