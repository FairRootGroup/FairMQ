################################################################################
# Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

if(NOT CTEST_SOURCE_DIRECTORY)
  set(CTEST_SOURCE_DIRECTORY .)
endif()
if(NOT CTEST_BINARY_DIRECTORY)
  set(CTEST_BINARY_DIRECTORY build)
endif()
if(NOT CTEST_TEST_TIMEOUT)
  set(CTEST_TEST_TIMEOUT 3600)
endif()
set(CTEST_USE_LAUNCHERS ON)

if(NOT CTEST_SITE)
  cmake_host_system_information(RESULT fqdn QUERY FQDN)
  message(STATUS "Running on host: ${fqdn}")
  set(CTEST_SITE "${fqdn}")
endif()

if(NOT CTEST_BUILD_NAME)
  message(FATAL_ERROR "No CTEST_BUILD_NAME set, aborting")
endif()
if(NOT CTEST_CMAKE_GENERATOR)
  set(CTEST_CMAKE_GENERATOR "Ninja")
endif()

list(APPEND configure_options
  "-DDISABLE_COLOR=ON"
  "-DBUILD_NANOMSG_TRANSPORT=ON"
# "-DBUILD_OFI_TRANSPORT=ON"
  "-DBUILD_DDS_PLUGIN=ON"
  "-DBUILD_SDK=ON"
  "-DBUILD_SDK_COMMANDS=ON"
)

if(NOT DEFINED BUILD_PMIX_PLUGIN)
  set(BUILD_PMIX_PLUGIN ON)
endif()
list(APPEND configure_options "-DBUILD_PMIX_PLUGIN=${BUILD_PMIX_PLUGIN}")

if(NOT CTEST_MODEL)
  set(CTEST_MODEL Experimental)
endif()
if(NOT CMAKE_BUILD_TYPE)
  set(CTEST_BUILD_TYPE RelWithDebInfo)
elseif(CMAKE_BUILD_TYPE STREQUAL StaticAnalysis)
  list(APPEND configure_options "-DRUN_STATIC_ANALYSIS=ON")
endif()
set(CTEST_BUILD_CONFIGURATION ${CMAKE_BUILD_TYPE})

list(APPEND configure_options "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
if(CMAKE_INSTALL_PREFIX)
  list(APPEND configure_options "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
endif()

ctest_start(${CTEST_MODEL})
ctest_submit(PARTS Start)

macro(check_res step)
  if(res)
    ctest_submit(PARTS ${step} Done)
    return()
  else()
    ctest_submit(PARTS ${step})
  endif()
  unset(res)
endmacro()

ctest_configure(OPTIONS "${configure_options}" RETURN_VALUE res)
check_res(Configure)

ctest_build(RETURN_VALUE res)
check_res(Build)
if(CMAKE_BUILD_TYPE STREQUAL StaticAnalysis)
  ctest_submit(PARTS Done)
  return()
endif()

ctest_test(SCHEDULE_RANDOM ON PARALLEL_LEVEL 3 RETURN_VALUE res)
check_res(Test)

if(CMAKE_BUILD_TYPE STREQUAL Profile)
  ctest_coverage(BUILD "${CTEST_BINARY_DIRECTORY}" LABELS coverage RETURN_VALUE res)
  check_res(Coverage)
endif()

ctest_submit(PARTS Done)
