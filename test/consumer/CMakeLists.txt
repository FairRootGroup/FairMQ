################################################################################
# Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH       #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

# Minimal downstream project. It checks that an installed FairMQ can be found
# and used the way the README describes. Run it via check-install.sh.

cmake_minimum_required(VERSION 3.15...3.30 FATAL_ERROR)
project(FairMQConsumer LANGUAGES CXX)

find_package(FairCMakeModules REQUIRED)
include(FairFindPackage2)
find_package2(PUBLIC FairMQ REQUIRED)
find_package2_implicit_dependencies()

# Variables documented in the README
foreach(var IN ITEMS FairMQ_VERSION FairMQ_PREFIX FairMQ_PACKAGE_DEPENDENCIES
                     FairMQ_PACKAGE_COMPONENTS)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "${var} is not set by find_package(FairMQ)")
  endif()
endforeach()
if(NOT FairMQ_fairmq_FOUND STREQUAL "TRUE")
  message(FATAL_ERROR "FairMQ_fairmq_FOUND is not set")
endif()
message(STATUS "FairMQ_PACKAGE_COMPONENTS: ${FairMQ_PACKAGE_COMPONENTS}")

add_executable(consumer main.cxx)
target_link_libraries(consumer PRIVATE FairMQ::FairMQ)
