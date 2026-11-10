################################################################################
# Copyright (C) 2018-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################


# Project ######################################################################
cmake_minimum_required(VERSION 3.25...3.31 FATAL_ERROR)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(GitHelper)
get_git_version()

project(FairMQ VERSION ${PROJECT_VERSION} LANGUAGES CXX)
include(FairMQProjectSettings)
################################################################################


# Build options ################################################################
include(CMakeDependentOption)

# The dependent options below need BUILD_FAIRMQ. Keep the error for an
# explicit request, because cmake_dependent_option() would silently disable it.
if(NOT BUILD_FAIRMQ AND DEFINED BUILD_FAIRMQ)
  foreach(opt IN ITEMS BUILD_TESTING BUILD_EXAMPLES)
    if(${opt})
      message(FATAL_ERROR "Cannot enable build option ${opt}, depending option is not set: BUILD_FAIRMQ")
    endif()
  endforeach()
endif()

option(BUILD_FAIRMQ        "Build FairMQ library and devices." ON)
cmake_dependent_option(BUILD_TESTING  "Build tests."          OFF "BUILD_FAIRMQ" OFF)
cmake_dependent_option(BUILD_EXAMPLES "Build FairMQ examples." ON "BUILD_FAIRMQ" OFF)
option(BUILD_TIDY_TOOL     "Build the fairmq-tidy tool." OFF)
option(USE_EXTERNAL_GTEST  "Do not use bundled GTest. Not recommended." OFF)
option(FAIRMQ_DEBUG_MODE   "Compile in debug mode (may decrease performance)." OFF)
################################################################################


# Dependencies #################################################################
include(CTest)
include(FairMQDependencies)
################################################################################


# Targets ######################################################################
if(BUILD_FAIRMQ)
  add_subdirectory(fairmq)
endif()

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BUILD_TIDY_TOOL)
  add_subdirectory(fairmq/tidy)
endif()
################################################################################


# Package components ###########################################################
if(BUILD_FAIRMQ)
  list(APPEND PROJECT_PACKAGE_COMPONENTS fairmq)
endif()
if(BUILD_TESTING)
  list(APPEND PROJECT_PACKAGE_COMPONENTS tests)
endif()
if(BUILD_EXAMPLES)
  list(APPEND PROJECT_PACKAGE_COMPONENTS examples)
endif()
if(BUILD_TIDY_TOOL)
  list(APPEND PROJECT_PACKAGE_COMPONENTS tidy_tool)
endif()
################################################################################


# Installation #################################################################
if(BUILD_FAIRMQ)
  install(FILES cmake/FindZeroMQ.cmake
    DESTINATION ${PROJECT_INSTALL_CMAKEMODDIR}
  )
endif()
if(BUILD_TIDY_TOOL)
  install(FILES cmake/FairMQTidy.cmake
    DESTINATION ${PROJECT_INSTALL_CMAKEMODDIR}
  )
endif()

include(FairMQPackage)
install_cmake_package()
################################################################################


# Summary ######################################################################
include(FairMQSummary)

message(STATUS "${BWhite}${PROJECT_NAME}${CR} ${PROJECT_GIT_VERSION} from ${PROJECT_DATE}")
fair_summary_global_cxx_flags_standard()
fair_summary_build_types()
fair_summary_package_dependencies()
fairmq_summary_components()
fairmq_summary_static_analysis()
fairmq_summary_install_prefix()
fairmq_summary_debug_mode()
fairmq_summary_compile_definitions()
message(STATUS "  ")
################################################################################
