################################################################################
# Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

include(FairCMakeModules)
include(FairFindPackage2)
include(FairMQBundlePackageHelper)

if(BUILD_FAIRMQ OR BUILD_SDK)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package2(PUBLIC Threads REQUIRED)
  set(Threads_PREFIX "<unknown system prefix>")
endif()

if(BUILD_OFI_TRANSPORT)
  find_package2(PRIVATE asiofi REQUIRED VERSION 0.5)
  find_package2(PRIVATE OFI REQUIRED)
endif()

if(BUILD_SDK_COMMANDS)
  find_package2(PRIVATE Flatbuffers REQUIRED)
endif()

if(BUILD_DDS_PLUGIN OR BUILD_SDK)
  find_package2(PRIVATE DDS REQUIRED VERSION 3.5.13.7)
  set(DDS_Boost_COMPONENTS system log log_setup regex filesystem thread)
  set(DDS_Boost_VERSION 1.67)
endif()

if(BUILD_PMIX_PLUGIN)
  find_package2(PRIVATE PMIx REQUIRED VERSION 2.1.4)
endif()

if(BUILD_FAIRMQ OR BUILD_SDK)
  find_package2(PUBLIC FairLogger REQUIRED VERSION 1.6.0)
  find_package2(PUBLIC Boost REQUIRED VERSION 1.66
    COMPONENTS container program_options filesystem date_time regex
  )
endif()

if(BUILD_OFI_TRANSPORT OR BUILD_SDK OR BUILD_DDS_PLUGIN)
  set(__old ${CMAKE_FIND_PACKAGE_PREFER_CONFIG})
  set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
  find_package2(PUBLIC asio REQUIRED VERSION 1.18)
  set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ${__old})
  unset(__old)
endif()

if(BUILD_FAIRMQ)
  find_package2(PRIVATE ZeroMQ REQUIRED VERSION 4.1.4)
  if(NOT PicoSHA2_BUNDLED)
    build_bundled(PicoSHA2 extern/PicoSHA2)
  endif()
  find_package2(BUNDLED PicoSHA2 REQUIRED)
  set(PicoSHA2_VERSION "1.0.0")
endif()

if(BUILD_TESTING)
  if(NOT GTest_FOUND AND NOT GTest_BUNDLED AND NOT USE_EXTERNAL_GTEST)
    build_bundled(GTest extern/googletest)
  endif()
  find_package2(BUNDLED GTest REQUIRED)
  if(GTest_BUNDLED)
    set(GTest_VERSION "Apr 28 2021 @f5e592d8")
  endif()
endif()

if(BUILD_DOCS)
  find_package2(PRIVATE Doxygen REQUIRED VERSION 1.8.8
    COMPONENTS dot
  )
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
    elseif(${dep} STREQUAL asiofi)
      if(NOT asiofi_PREFIX AND asiofi_ROOT)
        set(asiofi_PREFIX ${asiofi_ROOT})
      endif()
    elseif(${dep} STREQUAL DDS)
      set(DDS_PREFIX "${DDS_INSTALL_PREFIX}")
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
    elseif(${dep} STREQUAL Flatbuffers)
      if(TARGET flatbuffers::flatbuffers)
        get_target_property(flatbuffers_include flatbuffers::flatbuffers INTERFACE_INCLUDE_DIRECTORIES)
      else()
        get_target_property(flatbuffers_include flatbuffers::flatbuffers_shared INTERFACE_INCLUDE_DIRECTORIES)
      endif()
      get_filename_component(Flatbuffers_PREFIX ${flatbuffers_include}/.. ABSOLUTE)
      unset(flatbuffers_include)
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
