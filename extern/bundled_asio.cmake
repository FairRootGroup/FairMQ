################################################################################
#     Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH   #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

if(NOT TARGET asio::headers)
  if(Git_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --depth 1 -- extern/asio
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  endif()

endif()
