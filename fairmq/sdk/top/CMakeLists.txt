################################################################################
#    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(exe "fairmq-top")
add_executable(${exe}
  main.cxx
  Top.h
  Top.cxx
)
target_link_libraries(${exe}
  PRIVATE
  Boost::boost
  Boost::program_options
  Threads::Threads
  Ncurses::ncursesw
  SDK
)
target_include_directories(${exe} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
)
install(TARGETS ${exe}
  EXPORT ${PROJECT_EXPORT_SET}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)
