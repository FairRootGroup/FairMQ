################################################################################
# Copyright (C) 2018-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

include(FairCMakeModules)
include(FairFindPackage2)
include(FairMQBundlePackageHelper)

if(BUILD_FAIRMQ)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package2(PUBLIC Threads REQUIRED)
  set(Threads_PREFIX "<system>")
endif()

if(BUILD_FAIRMQ OR BUILD_TIDY_TOOL)
  find_package2(PUBLIC FairLogger REQUIRED VERSION 1.6.0)
  find_package2(PUBLIC Boost REQUIRED VERSION 1.66
    COMPONENTS container program_options filesystem date_time regex
  )
endif()

if(BUILD_FAIRMQ)
  find_package2(PRIVATE ZeroMQ REQUIRED VERSION 4.1.4)
  if(NOT PicoSHA2_BUNDLED)
    build_bundled(PicoSHA2 extern/PicoSHA2)
  endif()
  find_package2(BUNDLED PicoSHA2 REQUIRED)
  set(PicoSHA2_VERSION "1.0.0")
  set(PicoSHA2_PREFIX "<bundled>")
endif()

if(BUILD_TESTING)
  if(NOT GTest_FOUND AND NOT GTest_BUNDLED AND NOT USE_EXTERNAL_GTEST)
    build_bundled(GTest extern/googletest)
  endif()
  find_package2(BUNDLED GTest REQUIRED)
  if(GTest_BUNDLED)
    set(GTest_VERSION "Dec 26 2024 @7d76a23")
    set(GTest_PREFIX "<bundled>")
  endif()
endif()

if(BUILD_DOCS)
  find_package2(PRIVATE Doxygen REQUIRED VERSION 1.8.8
    COMPONENTS dot
  )
endif()

if(BUILD_TIDY_TOOL)
  find_package2(PRIVATE LLVM REQUIRED)
  find_package2(PRIVATE Clang REQUIRED)
  set(Clang_VERSION ${LLVM_VERSION})
  find_package2(PRIVATE CLI11 REQUIRED)
endif()

find_package2_implicit_dependencies() # Always call last after latest find_package2

if(PROJECT_PACKAGE_DEPENDENCIES)
  foreach(dep IN LISTS PROJECT_PACKAGE_DEPENDENCIES)
    string(TOUPPER ${dep} dep_upper)
    if(${dep}_BUNDLED)
      set(${dep}_PREFIX "<bundled>")
    elseif(${dep} STREQUAL FairLogger)
      if(NOT FairLogger_PREFIX AND FairLogger_ROOT)
        set(FairLogger_PREFIX ${FairLogger_ROOT})
      endif()
    elseif(${dep} STREQUAL Boost)
      if(TARGET Boost::headers)
        get_target_property(boost_include Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
      else()
        get_target_property(boost_include Boost::boost INTERFACE_INCLUDE_DIRECTORIES)
      endif()
      get_filename_component(Boost_PREFIX ${boost_include}/.. ABSOLUTE)
      unset(boost_include)
    elseif(${dep} STREQUAL Doxygen)
      get_target_property(doxygen_bin Doxygen::doxygen INTERFACE_LOCATION)
      get_filename_component(Doxygen_PREFIX ${doxygen_bin} DIRECTORY)
      get_filename_component(Doxygen_PREFIX ${Doxygen_PREFIX}/.. ABSOLUTE)
      unset(doxygen_bin)
    elseif(NOT ${dep}_PREFIX)
      # try to guess
      if(TARGET ${dep}::${dep})
        get_target_property(${dep}_include ${dep}::${dep} INTERFACE_INCLUDE_DIRECTORIES)
        get_filename_component(${dep}_PREFIX ${${dep}_include}/.. ABSOLUTE)
        unset(${dep}_include)
      elseif(${dep}_INCLUDE_DIR)
        get_filename_component(${dep}_PREFIX ${${dep}_INCLUDE_DIR}/.. ABSOLUTE)
      elseif(${dep_upper}_INCLUDE_DIR)
        get_filename_component(${dep}_PREFIX ${${dep_upper}_INCLUDE_DIR}/.. ABSOLUTE)
      elseif(${dep}_INCLUDE_DIRS)
        list(GET ${dep}_INCLUDE_DIRS 0 ${dep}_include)
        get_filename_component(${dep}_PREFIX ${${dep}_include}/.. ABSOLUTE)
        unset(${dep}_include)
      elseif(${dep_upper}_INCLUDE_DIRS)
        list(GET ${dep_upper}_INCLUDE_DIRS 0 ${dep}_include)
        get_filename_component(${dep}_PREFIX ${${dep}_include}/.. ABSOLUTE)
        unset(${dep}_include)
      endif()
    endif()
  endforeach()
endif()
