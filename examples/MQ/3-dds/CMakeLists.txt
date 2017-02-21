 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/3-dds/ex3-dds-topology.xml
               ${CMAKE_BINARY_DIR}/bin/config/ex3-dds-topology.xml @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/3-dds/ex3-dds-hosts.cfg
               ${CMAKE_BINARY_DIR}/bin/config/ex3-dds-hosts.cfg COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/examples/MQ/3-dds/ex3-dds.json
               ${CMAKE_BINARY_DIR}/bin/config/ex3-dds.json COPYONLY)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/examples/MQ/3-dds
  ${CMAKE_CURRENT_BINARY_DIR}
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${SYSTEM_INCLUDE_DIRECTORIES}
  ${Boost_INCLUDE_DIR}
  ${ZeroMQ_INCLUDE_DIR}
)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${LINK_DIRECTORIES}
  ${Boost_LIBRARY_DIRS}
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
)

set(LIBRARY_NAME FairMQExample3)

GENERATE_LIBRARY()

Set(Exe_Names
  ${Exe_Names}
  ex3-sampler
  ex3-processor
  ex3-sink
)

Set(Exe_Source
  ${Exe_Source}
  runExample3Sampler.cxx
  runExample3Processor.cxx
  runExample3Sink.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(BIN_DESTINATION share/fairbase/examples/MQ/3-dds/bin)
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/examples/MQ/3-dds")

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQExample3)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})

Install(
  FILES ex3-dds-hosts.cfg ex3-dds-topology.xml
  DESTINATION share/fairbase/examples/MQ/3-dds/config/
)
