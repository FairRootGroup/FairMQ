 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             # 
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #  
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/plugins/config
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
  "FairMQDDSConfigPlugin.cxx"
)

set(LIBRARY_NAME FairMQDDSConfigPlugin)

set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
  pthread
  ${DDS_INTERCOM_LIBRARY_SHARED}
  ${DDS_PROTOCOL_LIBRARY_SHARED}
  ${DDS_USER_DEFAULTS_LIBRARY_SHARED}
)

GENERATE_LIBRARY()
