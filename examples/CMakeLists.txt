################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

add_subdirectory(1-1)
add_subdirectory(1-n-1)
add_subdirectory(copypush)
add_subdirectory(dds)
add_subdirectory(multipart)
add_subdirectory(multiple-channels)
if(BUILD_NANOMSG_TRANSPORT)
  add_subdirectory(multiple-transports)
endif()
add_subdirectory(region)
add_subdirectory(req-rep)
