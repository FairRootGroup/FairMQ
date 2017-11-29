 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/fairmq/shmem/prototype/shm-prototype.json
               ${CMAKE_BINARY_DIR}/bin/config/shm-prototype.json)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/shmem/prototype/startShmPrototype.sh.in
               ${CMAKE_BINARY_DIR}/bin/prototype/shmem/startShmPrototype.sh)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/fairmq/shmem/prototype
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
  "FairMQShmPrototypeSampler.cxx"
  "FairMQShmPrototypeSink.cxx"
)

Set(DEPENDENCIES
  ${DEPENDENCIES}
  ${Boost_INTERPROCESS_LIBRARY}
  FairMQ
)

Set(LIBRARY_NAME FairMQShmPrototype)

GENERATE_LIBRARY()

Set(Exe_Names
  shm-prototype-sampler
  shm-prototype-sink
)

Set(Exe_Source
  runShmPrototypeSampler.cxx
  runShmPrototypeSink.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/prototype/shmem")

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQShmPrototype)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})
