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

if(BUILD_FAIRMQ)
  find_package2(PUBLIC FairLogger REQUIRED VERSION 1.6.0)
  find_package2(PUBLIC Boost REQUIRED VERSION 1.66
    COMPONENTS container program_options filesystem date_time regex
  )

  # Check Boost.Process compatibility
  # Boost 1.88 has broken Boost.Process v2 without v1 compatibility headers
  # Boost 1.89+ provides <boost/process/v1.hpp> for the old API
  if(Boost_VERSION VERSION_EQUAL "1.88.0")
    message(FATAL_ERROR "Boost version 1.88 is not supported due to Boost.Process API changes. "
                        "Please use Boost < 1.88 or >= 1.89")
  endif()

  if(Boost_VERSION VERSION_GREATER_EQUAL "1.89")
    set(FAIRMQ_BOOST_PROCESS_V1_HEADER ON CACHE INTERNAL "Use boost/process/v1.hpp for Boost >= 1.89")
  endif()
endif()

if(BUILD_FAIRMQ)
  find_package2(PRIVATE ZeroMQ REQUIRED VERSION 4.1.4)
  # Header-only, always bundled
  fairmq_init_submodule(extern/PicoSHA2)
  set(PicoSHA2_ROOT ${PROJECT_SOURCE_DIR}/extern/PicoSHA2)
  set(PicoSHA2_BUNDLED ON)
  find_package2(BUNDLED PicoSHA2 REQUIRED)
  set(PicoSHA2_VERSION "1.0.0")
  set(PicoSHA2_PREFIX "<bundled>")
endif()

if(BUILD_TESTING)
  if(USE_EXTERNAL_GTEST)
    find_package2(PRIVATE GTest REQUIRED)
  else()
    # Build the bundled GoogleTest as part of this build, with the same
    # compiler and flags. OVERRIDE_FIND_PACKAGE lets find_package(GTest)
    # below find it.
    fairmq_init_submodule(extern/googletest)
    include(FetchContent)
    FetchContent_Declare(GTest
      SOURCE_DIR ${PROJECT_SOURCE_DIR}/extern/googletest
      OVERRIDE_FIND_PACKAGE
    )
    block()
      set(BUILD_GMOCK OFF)
      set(INSTALL_GTEST OFF)
      set(BUILD_SHARED_LIBS OFF)
      set(CMAKE_POSITION_INDEPENDENT_CODE ON)
      # Do not analyse third-party code
      set(CMAKE_CXX_CLANG_TIDY "")
      set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "")
      set(CMAKE_CXX_CPPLINT "")
      FetchContent_MakeAvailable(GTest)
    endblock()
    set(GTest_BUNDLED ON)
    find_package2(BUNDLED GTest REQUIRED)
    set(GTest_VERSION "Dec 26 2024 @7d76a23")
    set(GTest_PREFIX "<bundled>")
  endif()
endif()

if(BUILD_TIDY_TOOL)
  enable_language(C) # LLVMConfig.cmake runs C compile checks
  find_package2(PRIVATE LLVM REQUIRED)
  find_package2(PRIVATE Clang REQUIRED CONFIG HINTS "${LLVM_LIBRARY_DIR}/cmake/clang")
  set(Clang_VERSION ${LLVM_VERSION})
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
      get_target_property(boost_include Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
      get_filename_component(Boost_PREFIX ${boost_include}/.. ABSOLUTE)
      unset(boost_include)
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
