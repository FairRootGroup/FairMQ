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
message(STATUS "${BWhite}${PROJECT_NAME}${CR} ${${PROJECT_NAME}_GIT_VERSION} from ${${PROJECT_NAME}_DATE}")

set_fairmq_defaults()

include(CTest)
################################################################################


# Build options ################################################################
option(BUILD_NANOMSG_TRANSPORT "Build nanomsg transport." OFF)
option(BUILD_OFI_TRANSPORT "Build experimental OFI transport." OFF)
option(BUILD_DDS_PLUGIN "Build DDS plugin." OFF)
################################################################################


# Dependencies #################################################################
set(dep Boost)
set(PROJECT_${dep}_VERSION 1.64)
set(PROJECT_${dep}_COMPONENTS program_options thread system filesystem regex date_time signals)
if(Boost_INCLUDE_DIR) # Silence output, if Boost was found already
  set(SILENCE_BOOST QUIET)
endif()
find_package(${dep} ${PROJECT_${dep}_VERSION} ${SILENCE_BOOST} REQUIRED COMPONENTS ${PROJECT_${dep}_COMPONENTS})
list(APPEND PROJECT_PACKAGE_DEPENDENCIES ${dep})

set(dep FairLogger)
set(PROJECT_${dep}_VERSION 1.0.2)
string(TOUPPER ${dep} upper_dep)
set(old ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH ${${upper_dep}_ROOT} $ENV{${upper_dep}_ROOT} ${CMAKE_PREFIX_PATH})
find_package(${dep} ${PROJECT_${dep}_VERSION} REQUIRED)
list(APPEND PROJECT_PACKAGE_DEPENDENCIES ${dep})
set(CMAKE_PREFIX_PATH ${old})

set(dep ZeroMQ)
set(PROJECT_${dep}_VERSION 4.2.2)
find_package(${dep} ${PROJECT_${dep}_VERSION} REQUIRED)
list(APPEND PROJECT_PACKAGE_DEPENDENCIES ${dep})

if(BUILD_NANOMSG_TRANSPORT)
  set(dep nanomsg)
  find_package(${dep} REQUIRED)
  list(APPEND PROJECT_PACKAGE_DEPENDENCIES ${dep})

  set(dep msgpack)
  set(PROJECT_${dep}_VERSION 2.1.5)
  string(TOUPPER ${dep} upper_dep)
  set(old ${CMAKE_PREFIX_PATH})
  set(CMAKE_PREFIX_PATH ${${upper_dep}_ROOT} $ENV{${upper_dep}_ROOT} ${CMAKE_PREFIX_PATH})
  find_package(${dep} ${PROJECT_${dep}_VERSION} REQUIRED)
  list(APPEND PROJECT_PACKAGE_DEPENDENCIES ${dep})
  set(CMAKE_PREFIX_PATH ${old})
endif()

if(BUILD_OFI_TRANSPORT)
  set(dep OFI)
  set(PROJECT_${dep}_VERSION 1.6.0)
  set(PROJECT_${dep}_COMPONENTS fi_sockets)
  find_package(${dep} ${PROJECT_${dep}_VERSION} REQUIRED COMPONENTS ${PROJECT_${dep}_COMPONENTS})
  list(APPEND PROJECT_PACKAGE_DEPENDENCIES ${dep})
endif()

if(BUILD_DDS_PLUGIN)
  set(dep DDS)
  set(PROJECT_${dep}_VERSION 2.0)
  find_package(${dep} ${PROJECT_${dep}_VERSION} REQUIRED)
  list(APPEND PROJECT_PACKAGE_DEPENDENCIES ${dep})
endif()

if(BUILD_TESTING)
  set(dep GTest)
  set(PROJECT_${dep}_VERSION 1.7.0)
  find_package(${dep} ${PROJECT_${dep}_VERSION} REQUIRED)
  list(APPEND PROJECT_PACKAGE_DEPENDENCIES ${dep})
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
install(FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/Version.h
  DESTINATION ${PROJECT_INSTALL_INCDIR}
)

install_cmake_package()
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
if(BUILD_NANOMSG_TRANSPORT)
  set(nn_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_NANOMSG_TRANSPORT=OFF${CR})")
else()
  set(nn_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_NANOMSG_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}nanomsg transport${CR}  ${nn_summary}")
if(BUILD_OFI_TRANSPORT)
  set(ofi_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_OFI_TRANSPORT=OFF${CR})")
else()
  set(ofi_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_OFI_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}OFI transport${CR}      ${ofi_summary} Experimental")
if(BUILD_DDS_PLUGIN)
  set(dds_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_DDS_PLUGIN=OFF${CR})")
else()
  set(dds_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_DDS_PLUGIN=ON${CR})")
endif()
message(STATUS "  ${BWhite}DDS plugin${CR}         ${dds_summary}")
message(STATUS "  ")
################################################################################
