################################################################################
#    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

include(FairMQBundlePackageHelper)

set(PROJECT_FairCMakeModules_VERSION 0.2)
find_package(FairCMakeModules ${PROJECT_FairCMakeModules_VERSION} QUIET)

if(NOT FairCMakeModules_FOUND AND NOT FairCMakeModules_BUNDLED)
  # FairCMakeModules expects to be the top-level project, so it cannot be added
  # with add_subdirectory(). Install the bundled copy into the build directory
  # at configure time instead. It contains only CMake modules.
  message(STATUS "Building bundled FairCMakeModules")
  fairmq_init_submodule(extern/FairCMakeModules)
  set(_fcm_src ${PROJECT_SOURCE_DIR}/extern/FairCMakeModules)
  set(_fcm_bin ${PROJECT_BINARY_DIR}/extern/FairCMakeModules)
  set(_fcm_prefix ${PROJECT_BINARY_DIR}/extern/FairCMakeModules_install)
  set(_fcm_log ${_fcm_bin}/build.log)
  file(MAKE_DIRECTORY ${_fcm_bin})
  file(REMOVE ${_fcm_log})
  foreach(_fcm_step IN ITEMS configure install)
    if(_fcm_step STREQUAL configure)
      set(_fcm_cmd ${CMAKE_COMMAND} -S ${_fcm_src} -B ${_fcm_bin} -G ${CMAKE_GENERATOR}
                   -DCMAKE_INSTALL_PREFIX=${_fcm_prefix}
                   -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                   -DBUILD_TESTING=OFF)
    else()
      set(_fcm_cmd ${CMAKE_COMMAND} --build ${_fcm_bin} --target install)
    endif()
    execute_process(COMMAND ${_fcm_cmd}
      OUTPUT_VARIABLE _fcm_out
      ERROR_VARIABLE _fcm_out
      RESULT_VARIABLE _fcm_res
    )
    file(APPEND ${_fcm_log} "${_fcm_out}")
    if(_fcm_res)
      message(FATAL_ERROR "Building bundled FairCMakeModules failed. See ${_fcm_log}")
    endif()
  endforeach()
  set(FairCMakeModules_ROOT ${_fcm_prefix} CACHE PATH "Location of bundled package FairCMakeModules")
  set(FairCMakeModules_BUNDLED ON CACHE BOOL "Whether bundled FairCMakeModules was used")
  message(STATUS "Building bundled FairCMakeModules - done")
  find_package(FairCMakeModules REQUIRED)
endif()
