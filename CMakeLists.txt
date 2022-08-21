################################################################################
# Copyright (C) 2018-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################


# Project ######################################################################
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
cmake_policy(VERSION 3.15...3.22)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(GitHelper)
get_git_version()

project(FairMQ VERSION ${PROJECT_VERSION} LANGUAGES CXX)
include(FairMQProjectSettings)
################################################################################


# Build options ################################################################
include(FairMQBuildOption)

fairmq_build_option(BUILD_FAIRMQ        "Build FairMQ library and devices."
                                         DEFAULT ON)
fairmq_build_option(BUILD_TESTING       "Build tests."
                                         DEFAULT OFF REQUIRES "BUILD_FAIRMQ")
fairmq_build_option(BUILD_OFI_TRANSPORT "Build experimental OFI transport."
                                         DEFAULT OFF REQUIRES "BUILD_FAIRMQ")
fairmq_build_option(BUILD_PMIX_PLUGIN   "Build PMIx plugin."
                                         DEFAULT OFF REQUIRES "BUILD_FAIRMQ")
fairmq_build_option(BUILD_EXAMPLES      "Build FairMQ examples."
                                         DEFAULT ON  REQUIRES "BUILD_FAIRMQ")
fairmq_build_option(BUILD_TIDY_TOOL     "Build the fairmq-tidy tool."
                                         DEFAULT OFF)
fairmq_build_option(BUILD_DOCS          "Build FairMQ documentation."
                                         DEFAULT OFF)
fairmq_build_option(USE_EXTERNAL_GTEST  "Do not use bundled GTest. Not recommended."
                                         DEFAULT OFF)
fairmq_build_option(FAIRMQ_DEBUG_MODE   "Compile in debug mode (may decrease performance)."
                                         DEFAULT OFF)
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

if(BUILD_DOCS)
  set(DOXYGEN_OUTPUT_DIRECTORY doxygen)
  set(DOXYGEN_PROJECT_NUMBER ${PROJECT_GIT_VERSION})
  set(DOXYGEN_PROJECT_BRIEF "C++ Message Queuing Library and Framework")
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  set(DOXYGEN_HTML_FOOTER docs/footer.html)
  doxygen_add_docs(doxygen README.md fairmq)
  add_custom_target(docs ALL DEPENDS doxygen)
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
if(BUILD_PMIX_PLUGIN)
  list(APPEND PROJECT_PACKAGE_COMPONENTS pmix_plugin)
endif()
if(BUILD_OFI_TRANSPORT)
  list(APPEND PROJECT_PACKAGE_COMPONENTS ofi_transport)
endif()
if(BUILD_EXAMPLES)
  list(APPEND PROJECT_PACKAGE_COMPONENTS examples)
endif()
if(BUILD_DOCS)
  list(APPEND PROJECT_PACKAGE_COMPONENTS docs)
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
if(BUILD_DOCS)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doxygen/html
    DESTINATION ${PROJECT_INSTALL_DATADIR}/docs
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
