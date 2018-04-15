################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

cmake_minimum_required(VERSION 3.9.4 FATAL_ERROR)


# Project ######################################################################
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FairMQLib)

set_fairmq_cmake_policies()
get_git_version(OUTVAR_PREFIX FairMQ)

project(FairMQ VERSION ${FairMQ_VERSION} LANGUAGES CXX)
message(STATUS "${BWhite}${PROJECT_NAME}${CR} ${FairMQ_GIT_VERSION} from ${FairMQ_DATE}")

set_fairmq_defaults()

include(CTest)
################################################################################


# Build options ################################################################
option(BUILD_NANOMSG_TRANSPORT "Build nanomsg transport." OFF)
option(BUILD_OFI_TRANSPORT "Build experimental OFI transport." OFF)
option(BUILD_DDS_PLUGIN "Build DDS plugin." OFF)
################################################################################


# Dependencies #################################################################
if(Boost_INCLUDE_DIR) # Silence output, if Boost was found already
  set(SILENCE_BOOST QUIET)
endif()
find_package(Boost 1.64 ${SILENCE_BOOST} REQUIRED
  COMPONENTS program_options thread system filesystem regex date_time signals
)

set(CMAKE_PREFIX_PATH ${FAIRLOGGER_ROOT} $ENV{FAIRLOGGER_ROOT} ${CMAKE_PREFIX_PATH})
find_package(FairLogger 1.0.1 REQUIRED)

find_package(ZeroMQ 4.2.2 REQUIRED)

if(BUILD_NANOMSG_TRANSPORT)
  find_package(nanomsg REQUIRED)
  set(CMAKE_PREFIX_PATH ${MSGPACK_ROOT} $ENV{MSGPACK_ROOT} ${CMAKE_PREFIX_PATH})
  find_package(msgpack 2.1.5 REQUIRED)
endif()

if(BUILD_OFI_TRANSPORT)
  find_package(OFI 1.6.0 REQUIRED COMPONENTS fi_sockets)
endif()

if(BUILD_DDS_PLUGIN)
  find_package(DDS 2.0 REQUIRED)
endif()

if(BUILD_TESTING)
  find_package(GTest REQUIRED)
endif()
################################################################################


# Targets ######################################################################
# Configure Version.hpp
configure_file(${PROJECT_NAME_LOWER}/Version.h.in
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/Version.h
  @ONLY
)

add_subdirectory(fairmq)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
################################################################################


# Installation #################################################################
install_fairmq_cmake_package()
################################################################################


# Summary ######################################################################
message(STATUS "  ")
message(STATUS "  ${Cyan}COMPONENT         BUILT?  INFO${CR}")
message(STATUS "  ${BWhite}library${CR}            ${BGreen}YES${CR}    (default, always built)")
if(BUILD_TESTING)
  set(tests_summary "${BGreen}YES${CR}    (default, disable with ${BMagenta}-DBUILD_TESTING=OFF${CR})")
else()
  set(tests_summary "${BRed} NO${CR}    (enable with ${BMagenta}-DBUILD_TESTING=ON${CR})")
endif()
message(STATUS "  ${BWhite}tests${CR}              ${tests_summary}")
if(BUILD_OFI_TRANSPORT)
  set(ofi_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_OFI_TRANSPORT=OFF${CR})")
else()
  set(ofi_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_OFI_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}OFI transport${CR}      ${ofi_summary} Experimental")
if(BUILD_NANOMSG_TRANSPORT)
  set(nn_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_NANOMSG_TRANSPORT=OFF${CR})")
else()
  set(nn_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_NANOMSG_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}nanomsg transport${CR}  ${nn_summary}")
if(BUILD_DDS_PLUGIN)
  set(dds_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_DDS_PLUGIN=OFF${CR})")
else()
  set(dds_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_DDS_PLUGIN=ON${CR})")
endif()
message(STATUS "  ${BWhite}DDS plugin${CR}         ${dds_summary}")
message(STATUS "  ")
################################################################################
