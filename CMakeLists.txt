################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
cmake_policy(VERSION 3.12...3.15)

# Project ######################################################################
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FairMQLib)

get_git_version()

project(FairMQ VERSION ${PROJECT_VERSION} LANGUAGES CXX)
message(STATUS "${BWhite}${PROJECT_NAME}${CR} ${PROJECT_GIT_VERSION} from ${PROJECT_DATE}")

if(BUILD_OFI_TRANSPORT OR BUILD_SDK OR BUILD_PMIX_PLUGIN)
  set(PROJECT_MIN_CXX_STANDARD 14)
else()
  set(PROJECT_MIN_CXX_STANDARD 11)
endif()

set_fairmq_defaults()

include(CTest)
################################################################################


# Build options ################################################################
fairmq_build_option(BUILD_FAIRMQ            "Build FairMQ library and devices."
                                             DEFAULT ON)
fairmq_build_option(BUILD_TESTING           "Build tests."
                                             DEFAULT OFF REQUIRES "BUILD_FAIRMQ")
fairmq_build_option(BUILD_OFI_TRANSPORT     "Build experimental OFI transport."
                                             DEFAULT OFF REQUIRES "BUILD_FAIRMQ")
fairmq_build_option(BUILD_SDK_COMMANDS      "Build the FairMQ SDK commands."
                                             DEFAULT OFF)
fairmq_build_option(BUILD_DDS_PLUGIN        "Build DDS plugin."
                                             DEFAULT OFF REQUIRES "BUILD_FAIRMQ;BUILD_SDK_COMMANDS")
fairmq_build_option(BUILD_PMIX_PLUGIN       "Build PMIx plugin."
                                             DEFAULT OFF REQUIRES "BUILD_FAIRMQ;BUILD_SDK_COMMANDS")
fairmq_build_option(BUILD_EXAMPLES          "Build FairMQ examples."
                                             DEFAULT ON  REQUIRES "BUILD_FAIRMQ")
fairmq_build_option(BUILD_SDK               "Build the FairMQ controller SDK."
                                             DEFAULT OFF REQUIRES "BUILD_DDS_PLUGIN;BUILD_SDK_COMMANDS")
fairmq_build_option(BUILD_DOCS              "Build FairMQ documentation."
                                             DEFAULT OFF)
fairmq_build_option(FAST_BUILD              "Fast production build. Not recommended for development."
                                             DEFAULT OFF)
fairmq_build_option(USE_EXTERNAL_GTEST      "Do not use bundled GTest. Not recommended."
                                             DEFAULT OFF)
################################################################################


# Dependencies #################################################################
if(FAST_BUILD)
  include(cotire)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if(BUILD_OFI_TRANSPORT)
  find_package2(PRIVATE asiofi REQUIRED
    VERSION 0.3.1
  )
  find_package2(PRIVATE OFI REQUIRED
    ADD_REQUIREMENTS_OF asiofi
  )
endif()

if(BUILD_SDK_COMMANDS)
  find_package2(PRIVATE Flatbuffers REQUIRED)
endif()

if(BUILD_DDS_PLUGIN OR BUILD_SDK)
  find_package2(PRIVATE DDS REQUIRED
    VERSION 3.0
  )
  set(DDS_Boost_COMPONENTS system log log_setup regex filesystem thread)
  set(DDS_Boost_VERSION 1.67)
endif()

if(BUILD_PMIX_PLUGIN)
  find_package2(PRIVATE PMIx REQUIRED
    VERSION 2.1.4
  )
endif()

if(BUILD_FAIRMQ OR BUILD_SDK)
  find_package2(PUBLIC FairLogger REQUIRED
    VERSION 1.2.0
  )

  foreach(dep IN LISTS FairLogger_PACKAGE_DEPENDENCIES)
    if(NOT dep STREQUAL "Boost")
      find_package2(PUBLIC ${dep} REQUIRED VERSION ${FairLogger_${dep}_VERSION})
      set(PROJECT_${dep}_VERSION ${FairLogger_${dep}_VERSION})
    endif()
  endforeach()

  if(NOT DEFINED Boost_NO_BOOST_CMAKE AND CMAKE_VERSION VERSION_LESS 3.15)
    # Since Boost 1.70 a CMake package is shipped by default. Unfortunately, it has a number
    # of problems that are only fixed in Boost 1.71 or CMake 3.15. By default we skip the
    # BoostConfig lookup. This can be overridden on the command line via -DBoost_NO_BOOST_CMAKE=OFF
    set(Boost_NO_BOOST_CMAKE ON)
  endif()
  find_package2(PUBLIC Boost REQUIRED
    VERSION 1.66

    COMPONENTS
    container
    program_options
    filesystem
    date_time
    regex

    ADD_REQUIREMENTS_OF
    asiofi
    DDS
    FairLogger
  )

  # Normalize Boost version
  if(CMAKE_VERSION VERSION_LESS 3.15)
    set(Boost_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
  endif()
endif()

if(BUILD_SDK)
  find_package2(BUNDLED asio
    VERSION 1.13.0
  )
  if(NOT asio_FOUND)
    build_bundled(asio extern/asio)
    find_package2(PRIVATE asio REQUIRED)
  endif()
endif()

if(BUILD_FAIRMQ)
  find_package2(PRIVATE ZeroMQ REQUIRED
    VERSION 4.1.4
  )
endif()

if(BUILD_TESTING)
  if(USE_EXTERNAL_GTEST)
    find_package2(PRIVATE GTest VERSION 1.7.0)
  endif()
  if(NOT GTest_FOUND)
    build_bundled(GTest extern/googletest)
    find_package2(PRIVATE GTest REQUIRED)
  endif()
endif()

if(BUILD_DOCS)
  find_package2(PRIVATE Doxygen REQUIRED
    VERSION 1.8.8
    COMPONENTS dot
    OPTIONAL_COMPONENTS mscgen dia
  )
endif()
################################################################################


# Targets ######################################################################
if(BUILD_FAIRMQ OR BUILD_SDK)
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
################################################################################


# Package components ###########################################################
if(BUILD_FAIRMQ)
  list(APPEND PROJECT_PACKAGE_COMPONENTS fairmq)
endif()
if(BUILD_TESTING)
  list(APPEND PROJECT_PACKAGE_COMPONENTS tests)
endif()
if(BUILD_DDS_PLUGIN)
  list(APPEND PROJECT_PACKAGE_COMPONENTS dds_plugin)
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
if(BUILD_SDK)
  list(APPEND PROJECT_PACKAGE_COMPONENTS sdk)
endif()
if(BUILD_SDK_COMMANDS)
  list(APPEND PROJECT_PACKAGE_COMPONENTS sdk_commands)
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
if(BUILD_SDK)
  install(FILES cmake/Findasio.cmake
    DESTINATION ${PROJECT_INSTALL_CMAKEMODDIR}
  )
  if(asio_BUNDLED)
    install(TARGETS bundled_asio_headers EXPORT ${PROJECT_EXPORT_SET})
    install(DIRECTORY "${asio_BUILD_INCLUDE_DIR}/asio"
      DESTINATION ${asio_INSTALL_INCLUDE_DIR}
      PATTERN "Makefile.am" EXCLUDE
      PATTERN ".gitignore" EXCLUDE
    )
    install(FILES "${asio_BUILD_INCLUDE_DIR}/asio.hpp"
      DESTINATION ${asio_INSTALL_INCLUDE_DIR}
    )
  endif()
endif()

install_cmake_package()
################################################################################


# Summary ######################################################################
message(STATUS "  ")
message(STATUS "  ${Cyan}CXX STANDARD${CR}       ${BGreen}C++${CMAKE_CXX_STANDARD}${CR} (>= C++${PROJECT_MIN_CXX_STANDARD}, change with ${BMagenta}-DCMAKE_CXX_STANDARD=17${CR})")
if(CMAKE_CXX_FLAGS)
  message(STATUS "  ")
  message(STATUS "  ${Cyan}GLOBAL CXX FLAGS${CR}  ${BGreen}${CMAKE_CXX_FLAGS}${CR}")
endif()
if(CMAKE_CONFIGURATION_TYPES)
  message(STATUS "  ")
  message(STATUS "  ${Cyan}BUILD TYPE         CXX FLAGS${CR}")
  string(TOUPPER "${CMAKE_BUILD_TYPE}" selected_type)
  foreach(type IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${type}" type_upper)
    if(type_upper STREQUAL selected_type)
      pad("${type}" 18 " " type_padded)
      message(STATUS "${BGreen}* ${type_padded}${CMAKE_CXX_FLAGS_${type_upper}}${CR}")
    else()
      pad("${type}" 18 " " type_padded)
      message(STATUS "  ${BWhite}${type_padded}${CR}${CMAKE_CXX_FLAGS_${type_upper}}")
    endif()
    unset(type_padded)
    unset(type_upper)
  endforeach()
  message(STATUS "  ")
  message(STATUS "  (Change the build type with ${BMagenta}-DCMAKE_BUILD_TYPE=...${CR})")
endif()
if(PROJECT_PACKAGE_DEPENDENCIES)
  message(STATUS "  ")
  message(STATUS "  ${Cyan}DEPENDENCY FOUND     VERSION                   PREFIX${CR}")
  foreach(dep IN LISTS PROJECT_PACKAGE_DEPENDENCIES)
    if(${dep}_VERSION AND NOT ${dep}_VERSION STREQUAL "..")
      set(version_str "${BGreen}${${dep}_VERSION}${CR}")
    else()
      set(version_str "${BYellow}unknown${CR}")
    endif()
    if(PROJECT_${dep}_VERSION)
      set(version_req_str " (>= ${PROJECT_${dep}_VERSION})")
    endif()
    pad(${dep} 20 " " dep_padded)
    if(DISABLE_COLOR)
      pad("${version_str}${version_req_str}" 25 " " version_padded)
    else()
      pad("${version_str}${version_req_str}" 25 " " version_padded COLOR 1)
    endif()
    if(${dep} STREQUAL FairLogger)
      if(FairLogger_PREFIX)
        set(prefix ${FairLogger_PREFIX})
      else()
        set(prefix ${FairLogger_ROOT})
      endif()
    elseif(${dep} STREQUAL GTest)
      get_filename_component(prefix ${GTEST_INCLUDE_DIRS}/.. ABSOLUTE)
    elseif(${dep} STREQUAL asiofi)
      set(prefix ${asiofi_ROOT})
    elseif(${dep} STREQUAL OFI)
      get_filename_component(prefix ${${dep}_INCLUDE_DIRS}/.. ABSOLUTE)
    elseif(${dep} STREQUAL DDS)
      set(prefix "${DDS_INSTALL_PREFIX}")
    elseif(${dep} STREQUAL Boost)
      if(TARGET Boost::headers)
        get_target_property(boost_include Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
      else()
        get_target_property(boost_include Boost::boost INTERFACE_INCLUDE_DIRECTORIES)
      endif()
      get_filename_component(prefix ${boost_include}/.. ABSOLUTE)
    elseif(${dep} STREQUAL Doxygen)
      get_target_property(doxygen_bin Doxygen::doxygen INTERFACE_LOCATION)
      get_filename_component(prefix ${doxygen_bin} DIRECTORY)
      get_filename_component(prefix ${prefix}/.. ABSOLUTE)
    elseif(${dep} STREQUAL fmt)
      get_target_property(fmt_include fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
      get_filename_component(prefix ${fmt_include}/.. ABSOLUTE)
    elseif(${dep} STREQUAL Flatbuffers)
      if(TARGET flatbuffers::flatbuffers)
        get_target_property(flatbuffers_include flatbuffers::flatbuffers INTERFACE_INCLUDE_DIRECTORIES)
      else()
        get_target_property(flatbuffers_include flatbuffers::flatbuffers_shared INTERFACE_INCLUDE_DIRECTORIES)
      endif()
      get_filename_component(prefix ${flatbuffers_include}/.. ABSOLUTE)
    else()
      get_filename_component(prefix ${${dep}_INCLUDE_DIR}/.. ABSOLUTE)
    endif()
    if(NOT ${dep}_BUNDLED)
      message(STATUS "  ${BWhite}${dep_padded}${CR}${version_padded}${prefix}")
    endif()
    unset(version_str)
    unset(version_padded)
    unset(version_req_str)
  endforeach()
endif()
message(STATUS "  ")
message(STATUS "  ${Cyan}COMPONENT         BUILT?  INFO${CR}")
if(BUILD_FAIRMQ)
  set(fairmq_summary "${BGreen}YES${CR}    (default, disable with ${BMagenta}-DBUILD_FAIRMQ=OFF${CR})")
else()
  set(fairmq_summary "${BRed} NO${CR}    (enable with ${BMagenta}-DBUILD_FAIRMQ=ON${CR})")
endif()
message(STATUS "  ${BWhite}fairmq${CR}             ${fairmq_summary}")
if(BUILD_TESTING)
  set(tests_summary "${BGreen}YES${CR}    (default, disable with ${BMagenta}-DBUILD_TESTING=OFF${CR})")
else()
  set(tests_summary "${BRed} NO${CR}    (enable with ${BMagenta}-DBUILD_TESTING=ON${CR})")
endif()
message(STATUS "  ${BWhite}tests${CR}              ${tests_summary}")
if(BUILD_OFI_TRANSPORT)
  set(ofi_summary "${BGreen}YES${CR}    EXPERIMENTAL (requires C++14) (disable with ${BMagenta}-DBUILD_OFI_TRANSPORT=OFF${CR})")
else()
  set(ofi_summary "${BRed} NO${CR}    EXPERIMENTAL (requires C++14) (default, enable with ${BMagenta}-DBUILD_OFI_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}ofi_transport${CR}      ${ofi_summary}")
if(BUILD_DDS_PLUGIN)
  set(dds_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_DDS_PLUGIN=OFF${CR})")
else()
  set(dds_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_DDS_PLUGIN=ON${CR})")
endif()
message(STATUS "  ${BWhite}dds_plugin${CR}         ${dds_summary}")
if(BUILD_PMIX_PLUGIN)
  set(pmix_summary "${BGreen}YES${CR}    EXPERIMENTAL (requires C++14) (disable with ${BMagenta}-DBUILD_PMIX_PLUGIN=OFF${CR})")
else()
  set(pmix_summary "${BRed} NO${CR}    EXPERIMENTAL (requires C++14) (default, enable with ${BMagenta}-DBUILD_PMIX_PLUGIN=ON${CR})")
endif()
message(STATUS "  ${BWhite}pmix_plugin${CR}        ${pmix_summary}")
if(BUILD_EXAMPLES)
  set(examples_summary "${BGreen}YES${CR}    (default, disable with ${BMagenta}-DBUILD_EXAMPLES=OFF${CR})")
else()
  set(examples_summary "${BRed} NO${CR}    (enable with ${BMagenta}-DBUILD_EXAMPLES=ON${CR})")
endif()
message(STATUS "  ${BWhite}examples${CR}           ${examples_summary}")
if(BUILD_DOCS)
  set(docs_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_DOCS=OFF${CR})")
else()
  set(docs_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_DOCS=ON${CR})")
endif()
message(STATUS "  ${BWhite}docs${CR}               ${docs_summary}")
if(BUILD_SDK)
  set(sdk_summary "${BGreen}YES${CR}    EXPERIMENTAL (required C++14) (disable with ${BMagenta}-DBUILD_SDK=OFF${CR})")
else()
  set(sdk_summary "${BRed} NO${CR}    EXPERIMENTAL (required C++14) (default, enable with ${BMagenta}-DBUILD_SDK=ON${CR})")
endif()
message(STATUS "  ${BWhite}sdk${CR}                ${sdk_summary}")
if(BUILD_SDK_COMMANDS)
  set(sdk_commands_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_SDK_COMMANDS=OFF${CR})")
else()
  set(sdk_commands_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_SDK_COMMANDS=ON${CR})")
endif()
message(STATUS "  ${BWhite}sdk_commands${CR}       ${sdk_commands_summary}")
message(STATUS "  ")
if(RUN_STATIC_ANALYSIS)
  list(LENGTH PROJECT_STATIC_ANALYSERS size)
  unset(analyser_list)
  set(count 0)
  foreach(analyser IN LISTS PROJECT_STATIC_ANALYSERS)
    if(${analyser}_FOUND)
      set(${analyser}_status "${analyser} ${BGreen}YES${CR}")
    else()
      set(${analyser}_status "${analyser} ${BRed}NO${CR}")
    endif()
    math(EXPR count "${count} + 1")
    string(APPEND analyser_list "${${analyser}_status}")
    if(count LESS size)
      string(APPEND analyser_list "${BWhite},${CR} ")
    endif()
  endforeach()
  set(static_ana_summary "${BWhite}(${CR}${analyser_list}${BWhite})${CR}  (disable with ${BMagenta}-DRUN_STATIC_ANALYSIS=OFF${CR})")
else()
  set(static_ana_summary "${BRed}OFF${CR}  (default, enable with ${BMagenta}-DRUN_STATIC_ANALYSIS=ON${CR})")
endif()
message(STATUS "  ${Cyan}INSTALL PREFIX${CR}     ${BGreen}${CMAKE_INSTALL_PREFIX}${CR} (change with ${BMagenta}-DCMAKE_INSTALL_PREFIX=...${CR})")
message(STATUS "  ")
message(STATUS "  ${Cyan}RUN STATIC ANALYSIS  ${static_ana_summary}")
message(STATUS "  ")
################################################################################
