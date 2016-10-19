 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             # 
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #  
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/plugins/control
  )

set(SYSTEM_INCLUDE_DIRECTORIES
  ${SYSTEM_INCLUDE_DIRECTORIES}
  ${Boost_INCLUDE_DIR}
  ${DDS_INCLUDE_DIR}
)

include_directories(${INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

set(LINK_DIRECTORIES
  ${LINK_DIRECTORIES}
  ${Boost_LIBRARY_DIRS}
)

link_directories(${LINK_DIRECTORIES})

set(SRCS
  "FairMQDDSControlPlugin.cxx"
)

set(LIBRARY_NAME FairMQDDSControlPlugin)

set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
  pthread
  ${DDS_INTERCOM_LIBRARY_SHARED}
  ${DDS_PROTOCOL_LIBRARY_SHARED}
  ${DDS_USER_DEFAULTS_LIBRARY_SHARED}
)

GENERATE_LIBRARY()

set(Exe_Names
  fairmq-dds-command-ui
)

set(Exe_Source
  ../../run/runDDSCommandUI.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

foreach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQ pthread ${DDS_INTERCOM_LIBRARY_SHARED} ${DDS_PROTOCOL_LIBRARY_SHARED} ${DDS_USER_DEFAULTS_LIBRARY_SHARED})
  GENERATE_EXECUTABLE()
endforeach(_file RANGE 0 ${_length})
