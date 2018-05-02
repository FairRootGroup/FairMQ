################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################
Set(CTEST_SOURCE_DIRECTORY $ENV{SOURCEDIR})
Set(CTEST_BINARY_DIRECTORY $ENV{BUILDDIR})
Set(CTEST_SITE $ENV{SITE})
Set(CTEST_BUILD_NAME $ENV{LABEL})
Set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
Set(CTEST_PROJECT_NAME "FairMQ")

Find_Program(CTEST_GIT_COMMAND NAMES git)
Set(CTEST_UPDATE_COMMAND "${CTEST_GIT_COMMAND}")

Set(BUILD_COMMAND "make")
Set(CTEST_BUILD_COMMAND "${BUILD_COMMAND} -j$ENV{number_of_processors}")

String(TOUPPER $ENV{ctest_model} _Model)
Set(configure_options "-DCMAKE_BUILD_TYPE=${_Model}")

Set(CTEST_USE_LAUNCHERS 1)
Set(configure_options "${configure_options};-DCTEST_USE_LAUNCHERS=${CTEST_USE_LAUNCHERS}")

Set(configure_options "${configure_options};-DDISABLE_COLOR=ON")
Set(configure_options "${configure_options};-DCMAKE_PREFIX_PATH=$ENV{SIMPATH}")
Set(configure_options "${configure_options};-DBUILD_NANOMSG_TRANSPORT=ON")
Set(configure_options "${configure_options};-DBUILD_DDS_PLUGIN=ON")

Set(EXTRA_FLAGS $ENV{EXTRA_FLAGS})
If(EXTRA_FLAGS)
  Set(configure_options "${configure_options};${EXTRA_FLAGS}") 
EndIf()

If($ENV{ctest_model} MATCHES Nightly OR $ENV{ctest_model} MATCHES Profile)

  Find_Program(GCOV_COMMAND gcov)
  If(GCOV_COMMAND)
    Message("Found GCOV: ${GCOV_COMMAND}")
    Set(CTEST_COVERAGE_COMMAND ${GCOV_COMMAND})
  EndIf(GCOV_COMMAND)

  Set(ENV{ctest_model} Nightly)

  CTEST_EMPTY_BINARY_DIRECTORY(${CTEST_BINARY_DIRECTORY})

EndIf()

Ctest_Start($ENV{ctest_model})

Ctest_Configure(BUILD "${CTEST_BINARY_DIRECTORY}"
                OPTIONS "${configure_options}"
               )

Ctest_Build(BUILD "${CTEST_BINARY_DIRECTORY}")

Ctest_Test(BUILD "${CTEST_BINARY_DIRECTORY}" 
           PARALLEL_LEVEL $ENV{number_of_processors}
           RETURN_VALUE _ctest_test_ret_val
          )

If(GCOV_COMMAND)
  Ctest_Coverage(BUILD "${CTEST_BINARY_DIRECTORY}")
EndIf()

Ctest_Submit()
 
if (_ctest_test_ret_val)
  Message(FATAL_ERROR "Some tests failed.")
endif()
