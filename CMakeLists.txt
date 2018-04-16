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
get_git_version()

project(FairMQ VERSION ${PROJECT_VERSION} LANGUAGES CXX)
message(STATUS "${BWhite}${PROJECT_NAME}${CR} ${PROJECT_GIT_VERSION} from ${PROJECT_DATE}")

set_fairmq_defaults()

include(CTest)
################################################################################


# Build options ################################################################
option(BUILD_NANOMSG_TRANSPORT "Build nanomsg transport." OFF)
option(BUILD_OFI_TRANSPORT "Build experimental OFI transport." OFF)
option(BUILD_DDS_PLUGIN "Build DDS plugin." OFF)
################################################################################


# Dependencies #################################################################
find_package2(PUBLIC Boost VERSION 1.64 REQUIRED
  COMPONENTS program_options thread system filesystem regex date_time signals
)
find_package2(PUBLIC FairLogger VERSION 1.0.2 REQUIRED)
find_package2(PRIVATE ZeroMQ VERSION 4.2.2 REQUIRED)

if(BUILD_NANOMSG_TRANSPORT)
  find_package2(PRIVATE nanomsg VERSION 1.0.0 REQUIRED)
  find_package2(PRIVATE msgpack VERSION 2.1.5 REQUIRED)
endif()

if(BUILD_OFI_TRANSPORT)
  find_package2(PRIVATE OFI VERSION 1.6.0 REQUIRED COMPONENTS fi_sockets)
  find_package2(PRIVATE Protobuf VERSION 3.4.0 REQUIRED)
endif()

if(BUILD_DDS_PLUGIN)
  find_package2(PRIVATE DDS VERSION 2.0 REQUIRED)
endif()

if(BUILD_TESTING)
  find_package2(PRIVATE GTest VERSION 1.7.0 REQUIRED)
endif()
################################################################################


# Targets ######################################################################
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
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/Version.h
  DESTINATION ${PROJECT_INSTALL_INCDIR}
)

install_cmake_package()
################################################################################


# Summary ######################################################################
message(STATUS "  ")
message(STATUS "  ${Cyan}DEPENDENCY      VERSION              PREFIX${CR}")
foreach(dep IN LISTS PROJECT_PACKAGE_DEPENDENCIES)
  if(${dep}_VERSION)
    if(${dep} STREQUAL Boost)
      set(version_str "${BGreen}${${dep}_MAJOR_VERSION}.${${dep}_MINOR_VERSION}${CR}")
    else()
      set(version_str "${BGreen}${${dep}_VERSION}${CR}")
    endif()
  else()
    set(version_str "${BYellow}unknown${CR}")
  endif()
  if(PROJECT_${dep}_VERSION)
    set(version_req_str " (>= ${PROJECT_${dep}_VERSION})")
  endif()
  pad(${dep} 15 " " dep_padded)
  pad("${version_str}${version_req_str}" 20 " " version_padded)
  message(STATUS "  ${BWhite}${dep_padded}${CR}${version_padded}...")
  unset(version_str)
endforeach()
message(STATUS "  ")
message(STATUS "  ${Cyan}COMPONENT         BUILT?  INFO${CR}")
message(STATUS "  ${BWhite}FairMQ${CR}             ${BGreen}YES${CR}    (default, always built)")
if(BUILD_TESTING)
  set(tests_summary "${BGreen}YES${CR}    (default, disable with ${BMagenta}-DBUILD_TESTING=OFF${CR})")
else()
  set(tests_summary "${BRed} NO${CR}    (enable with ${BMagenta}-DBUILD_TESTING=ON${CR})")
endif()
message(STATUS "  ${BWhite}test${CR}               ${tests_summary}")
if(BUILD_NANOMSG_TRANSPORT)
  set(nn_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_NANOMSG_TRANSPORT=OFF${CR})")
else()
  set(nn_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_NANOMSG_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}NanomsgTransport${CR}   ${nn_summary}")
if(BUILD_OFI_TRANSPORT)
  set(ofi_summary "${BGreen}YES${CR}    EXPERIMENTAL (disable with ${BMagenta}-DBUILD_OFI_TRANSPORT=OFF${CR})")
else()
  set(ofi_summary "${BRed} NO${CR}    EXPERIMENTAL (default, enable with ${BMagenta}-DBUILD_OFI_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}OfiTransport${CR}       ${ofi_summary}")
if(BUILD_DDS_PLUGIN)
  set(dds_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_DDS_PLUGIN=OFF${CR})")
else()
  set(dds_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_DDS_PLUGIN=ON${CR})")
endif()
message(STATUS "  ${BWhite}FairMQPlugin_dds${CR}   ${dds_summary}")
message(STATUS "  ")
################################################################################
