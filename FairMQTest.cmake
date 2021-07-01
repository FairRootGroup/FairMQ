################################################################################
#    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

cmake_host_system_information(RESULT fqdn QUERY FQDN)

set(CTEST_SOURCE_DIRECTORY .)
set(CTEST_BINARY_DIRECTORY build)
set(CTEST_CMAKE_GENERATOR "Ninja")
set(CTEST_USE_LAUNCHERS ON)
set(CTEST_CONFIGURATION_TYPE "RelWithDebInfo")
set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE 102400)

if(NOT NCPUS)
  if(ENV{SLURM_CPUS_PER_TASK})
    set(NCPUS $ENV{SLURM_CPUS_PER_TASK})
  else()
    include(ProcessorCount)
    ProcessorCount(NCPUS)
    if(NCPUS EQUAL 0)
      set(NCPUS 1)
    endif()
  endif()
endif()

if ("$ENV{CTEST_SITE}" STREQUAL "")
  set(CTEST_SITE "${fqdn}")
else()
  set(CTEST_SITE $ENV{CTEST_SITE})
endif()

if ("$ENV{LABEL}" STREQUAL "")
  set(CTEST_BUILD_NAME "build")
else()
  set(CTEST_BUILD_NAME $ENV{LABEL})
endif()

ctest_start(Continuous)

list(APPEND options "-DDISABLE_COLOR=ON" "-DBUILD_EXAMPLES=ON" "-DBUILD_TESTING=ON")
if(HAS_ASIO AND HAS_DDS)
  list(APPEND options "-DBUILD_SDK_COMMANDS=ON" "-DBUILD_SDK=ON" "-DBUILD_DDS_PLUGIN=ON")
endif()
if(HAS_PMIX)
  list(APPEND options "-DBUILD_SDK_COMMANDS=ON" "-DBUILD_PMIX_PLUGIN=ON")
endif()
if(HAS_ASIO AND HAS_ASIOFI)
  list(APPEND options "-DBUILD_OFI_TRANSPORT=ON")
endif()
if(RUN_STATIC_ANALYSIS)
  list(APPEND options "-DRUN_STATIC_ANALYSIS=ON")
endif()
if(CMAKE_BUILD_TYPE)
  set(CTEST_CONFIGURATION_TYPE ${CMAKE_BUILD_TYPE})
endif()
if(ENABLE_SANITIZER_ADDRESS)
  list(APPEND options "-DENABLE_SANITIZER_ADDRESS=ON")
endif()
if(ENABLE_SANITIZER_LEAK)
  list(APPEND options "-DENABLE_SANITIZER_LEAK=ON")
endif()
if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
  list(APPEND options "-DENABLE_SANITIZER_UNDEFINED_BEHAVIOR=ON")
endif()
if(ENABLE_SANITIZER_MEMORY)
  list(APPEND options "-DENABLE_SANITIZER_MEMORY=ON")
endif()
if(ENABLE_SANITIZER_THREAD)
  list(APPEND options "-DENABLE_SANITIZER_THREAD=ON")
endif()
if(CMAKE_CXX_FLAGS)
  list(APPEND options "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
endif()
list(REMOVE_DUPLICATES options)
list(JOIN options ";" optionsstr)
ctest_configure(OPTIONS "${optionsstr}")

ctest_submit()

ctest_build(FLAGS "-j${NCPUS}")

ctest_submit()

if(NOT RUN_STATIC_ANALYSIS)
  ctest_test(BUILD "${CTEST_BINARY_DIRECTORY}"
             PARALLEL_LEVEL 1
             SCHEDULE_RANDOM ON
             RETURN_VALUE _ctest_test_ret_val)

  ctest_submit()
endif()

if(_ctest_test_ret_val)
  Message(FATAL_ERROR "Some tests failed.")
endif()
