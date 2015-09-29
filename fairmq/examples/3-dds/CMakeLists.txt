 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/3-dds/ex3-devices.json ${CMAKE_BINARY_DIR}/bin/config/ex3-devices.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/3-dds/ex3-dds-topology.xml ${CMAKE_BINARY_DIR}/bin/config/ex3-dds-topology.xml)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/examples/3-dds/ex3-dds-hosts.cfg ${CMAKE_BINARY_DIR}/bin/config/ex3-dds-hosts.cfg COPYONLY)

add_definitions(-DENABLE_DDS)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/fairmq/examples/3-dds
  ${CMAKE_CURRENT_BINARY_DIR}
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${SYSTEM_INCLUDE_DIRECTORIES}
  ${DDS_INCLUDE_DIR}
)

If(NANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  )
Else(NANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  )
EndIf(NANOMSG_FOUND)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${LINK_DIRECTORIES}
  ${Boost_LIBRARY_DIRS}
  ${DDS_LIBRARY_DIR}
)

Link_Directories(${LINK_DIRECTORIES})

Set(SRCS
  ${SRCS}
  "FairMQExample3Sampler.cxx"
  "FairMQExample3Processor.cxx"
  "FairMQExample3Sink.cxx"
)

Set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
  dds-key-value-lib
)

set(LIBRARY_NAME FairMQExample3)

GENERATE_LIBRARY()

Set(Exe_Names
  ${Exe_Names}
  ex3-sampler-dds
  ex3-processor-dds
  ex3-sink-dds
)

Set(Exe_Source
  ${Exe_Source}
  runExample3Sampler.cxx
  runExample3Processor.cxx
  runExample3Sink.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample3)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})
