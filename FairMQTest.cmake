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

list(APPEND options
  "-DDISABLE_COLOR=ON"
  "-DBUILD_SDK_COMMANDS=ON"
  "-DBUILD_SDK=ON"
  "-DBUILD_DDS_PLUGIN=ON"
  )
list(JOIN options ";" optionsstr)
ctest_configure(OPTIONS "${optionsstr}")

ctest_build(FLAGS "-j${NCPUS}")

ctest_test(BUILD "${CTEST_BINARY_DIRECTORY}"
           PARALLEL_LEVEL 1
           SCHEDULE_RANDOM ON
           RETURN_VALUE _ctest_test_ret_val)

ctest_submit()

if(_ctest_test_ret_val)
  Message(FATAL_ERROR "Some tests failed.")
endif()
