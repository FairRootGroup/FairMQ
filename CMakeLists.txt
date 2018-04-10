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
message("${BWhite}${PROJECT_NAME}${CR} ${FairMQ_GIT_VERSION} from ${FairMQ_DATE}")

set_fairmq_defaults()
################################################################################


# Build options ################################################################
option(BUILD_OFI_TRANSPORT "Build OFI transport." OFF)
################################################################################


# Dependencies #################################################################
if(Boost_INCLUDE_DIR) # Silence output, if Boost was found already
  set(SILENCE_BOOST QUIET)
endif()
find_package(Boost 1.64 ${SILENCE_BOOST} REQUIRED COMPONENTS system)

if(BUILD_OFI_TRANSPORT)
  find_package(OFI 1.6.0 REQUIRED COMPONENTS fi_sockets)
endif()
################################################################################


# Targets ######################################################################
# Configure Version.hpp
configure_file(${PROJECT_NAME_LOWER}/Version.h.in
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/Version.h
  @ONLY
)

add_subdirectory(fairmq)
################################################################################


# Installation #################################################################
install_fairmq_cmake_package()
################################################################################


# Summary ######################################################################
message("  ")
message("  ${Cyan}COMPONENT  BUILT?  INFO${CR}")
message("  ${BWhite}library${CR}          ${BGreen}YES${CR}    (default, always built)")
if(BUILD_OFI_TRANSPORT)
  set(ofi_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_OFI_TRANSPORT=OFF${CR})")
else()
  set(ofi_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_OFI_TRANSPORT=ON${CR})")
endif()
message("  ${BWhite}OFI transport${CR}    ${ofi_summary}")
message("  ")
################################################################################
