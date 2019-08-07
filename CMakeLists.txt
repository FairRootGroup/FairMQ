################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

cmake_minimum_required(VERSION 3.9.4 FATAL_ERROR)
cmake_policy(VERSION 3.9...3.15)

# Project ######################################################################
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FairMQLib)

get_git_version()

project(FairMQ VERSION ${PROJECT_VERSION} LANGUAGES CXX)
message(STATUS "${BWhite}${PROJECT_NAME}${CR} ${PROJECT_GIT_VERSION} from ${PROJECT_DATE}")

set_fairmq_defaults()

include(CTest)
################################################################################


# Build options ################################################################
include(CMakeDependentOption)
option(BUILD_FAIRMQ "Build FairMQ library and devices." ON)
cmake_dependent_option(BUILD_TESTING "Build tests." OFF "BUILD_FAIRMQ" OFF)
cmake_dependent_option(BUILD_NANOMSG_TRANSPORT "Build nanomsg transport." OFF "BUILD_FAIRMQ" OFF)
cmake_dependent_option(BUILD_OFI_TRANSPORT "Build experimental OFI transport." OFF "BUILD_FAIRMQ" OFF)
cmake_dependent_option(BUILD_DDS_PLUGIN "Build DDS plugin." OFF "BUILD_FAIRMQ" OFF)
cmake_dependent_option(BUILD_EXAMPLES "Build FairMQ examples." ON "BUILD_FAIRMQ" OFF)
option(BUILD_DOCS "Build FairMQ documentation." OFF)
option(FAST_BUILD "Fast production build. Not recommended for development." OFF)
################################################################################


# Dependencies #################################################################
if(FAST_BUILD)
  include(cotire)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if(BUILD_FAIRMQ)
  find_package2(PUBLIC Boost VERSION 1.64 REQUIRED
    COMPONENTS container program_options thread system filesystem regex date_time
  )
  # Normalize Boost version
  if(CMAKE_VERSION VERSION_LESS 3.15)
    set(Boost_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
  endif()
  find_package2(PUBLIC FairLogger VERSION 1.2.0 REQUIRED)
  find_package2(PRIVATE ZeroMQ VERSION 4.1.5 REQUIRED)
endif()

if(BUILD_NANOMSG_TRANSPORT)
  find_package2(PRIVATE nanomsg REQUIRED)
  set(PROJECT_nanomsg_VERSION 1.1.3) # Once upstream releases 1.1.5, we should bump again and use version check
  find_package2(PRIVATE msgpack VERSION 3.1.0 REQUIRED)
endif()

if(BUILD_OFI_TRANSPORT)
  find_package2(PRIVATE OFI VERSION 1.6.0 REQUIRED COMPONENTS fi_sockets fi_verbs)
  find_package2(PRIVATE Protobuf VERSION 3.4.0 REQUIRED)
endif()

if(BUILD_DDS_PLUGIN)
  find_package2(PRIVATE DDS VERSION 2.2 REQUIRED)
endif()

if(BUILD_TESTING)
  find_package2(PRIVATE GTest VERSION 1.7.0 REQUIRED)
endif()

if(BUILD_DOCS)
  find_package2(PRIVATE Doxygen VERSION 1.8.8 REQUIRED COMPONENTS dot OPTIONAL_COMPONENTS mscgen dia)
endif()
################################################################################


# Targets ######################################################################
if(BUILD_FAIRMQ)
  configure_file(${PROJECT_NAME_LOWER}/Version.h.in
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/Version.h
    @ONLY
  )

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
  set(DOXYGEN_PROJECT_BRIEF "C++ Message Passing Framework")
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
if(BUILD_NANOMSG_TRANSPORT)
  list(APPEND PROJECT_PACKAGE_COMPONENTS nanomsg_transport)
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
################################################################################


# Installation #################################################################
if(BUILD_FAIRMQ)
  install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME_LOWER}/Version.h
    DESTINATION ${PROJECT_INSTALL_INCDIR}
  )

  install(FILES cmake/FindZeroMQ.cmake
    DESTINATION ${PROJECT_INSTALL_CMAKEMODDIR}
  )
endif()
if(BUILD_DDS_PLUGIN)
  install(FILES cmake/FindDDS.cmake
    DESTINATION ${PROJECT_INSTALL_CMAKEMODDIR}
  )
endif()
if(BUILD_OFI_TRANSPORT)
  install(FILES cmake/FindOFI.cmake
    DESTINATION ${PROJECT_INSTALL_CMAKEMODDIR}
  )
endif()
if(BUILD_DOCS)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doxygen/html
    DESTINATION ${PROJECT_INSTALL_DATADIR}/docs
  )
endif()

install_cmake_package()
################################################################################


# Summary ######################################################################
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
    if(${dep}_VERSION)
      if(${dep} STREQUAL Boost)
        set(version_str "${BGreen}${${dep}_MAJOR_VERSION}.${${dep}_MINOR_VERSION}${CR}")
      else()
        set(version_str "${BGreen}${${dep}_VERSION}${CR}")
      endif()
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
    elseif(${dep} STREQUAL msgpack)
      get_target_property(msgpack_include msgpackc-cxx INTERFACE_INCLUDE_DIRECTORIES)
      get_filename_component(prefix ${msgpack_include}/.. ABSOLUTE)
    elseif(${dep} STREQUAL OFI)
      get_filename_component(prefix ${${dep}_INCLUDE_DIRS}/.. ABSOLUTE)
    elseif(${dep} STREQUAL nanomsg)
      get_target_property(nn_include nanomsg INTERFACE_INCLUDE_DIRECTORIES)
      get_filename_component(prefix ${nn_include}/.. ABSOLUTE)
    elseif(${dep} STREQUAL Doxygen)
      get_target_property(doxygen_bin Doxygen::doxygen INTERFACE_LOCATION)
      get_filename_component(prefix ${doxygen_bin} DIRECTORY)
      get_filename_component(prefix ${prefix}/.. ABSOLUTE)
    else()
      get_filename_component(prefix ${${dep}_INCLUDE_DIR}/.. ABSOLUTE)
    endif()
    message(STATUS "  ${BWhite}${dep_padded}${CR}${version_padded}${prefix}")
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
if(BUILD_NANOMSG_TRANSPORT)
  set(nn_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_NANOMSG_TRANSPORT=OFF${CR})")
else()
  set(nn_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_NANOMSG_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}nanomsg_transport${CR}  ${nn_summary}")
if(BUILD_OFI_TRANSPORT)
  set(ofi_summary "${BGreen}YES${CR}    EXPERIMENTAL (disable with ${BMagenta}-DBUILD_OFI_TRANSPORT=OFF${CR})")
else()
  set(ofi_summary "${BRed} NO${CR}    EXPERIMENTAL (default, enable with ${BMagenta}-DBUILD_OFI_TRANSPORT=ON${CR})")
endif()
message(STATUS "  ${BWhite}ofi_transport${CR}      ${ofi_summary}")
if(BUILD_DDS_PLUGIN)
  set(dds_summary "${BGreen}YES${CR}    (disable with ${BMagenta}-DBUILD_DDS_PLUGIN=OFF${CR})")
else()
  set(dds_summary "${BRed} NO${CR}    (default, enable with ${BMagenta}-DBUILD_DDS_PLUGIN=ON${CR})")
endif()
message(STATUS "  ${BWhite}dds_plugin${CR}         ${dds_summary}")
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
message(STATUS "  ")
################################################################################
