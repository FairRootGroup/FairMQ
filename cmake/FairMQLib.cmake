################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

### PUBLIC

# Defines some variables with console color escape sequences
if(NOT WIN32 AND NOT DISABLE_COLOR)
  string(ASCII 27 Esc)
  set(CR       "${Esc}[m")
  set(CB       "${Esc}[1m")
  set(Red      "${Esc}[31m")
  set(Green    "${Esc}[32m")
  set(Yellow   "${Esc}[33m")
  set(Blue     "${Esc}[34m")
  set(Magenta  "${Esc}[35m")
  set(Cyan     "${Esc}[36m")
  set(White    "${Esc}[37m")
  set(BRed     "${Esc}[1;31m")
  set(BGreen   "${Esc}[1;32m")
  set(BYellow  "${Esc}[1;33m")
  set(BBlue    "${Esc}[1;34m")
  set(BMagenta "${Esc}[1;35m")
  set(BCyan    "${Esc}[1;36m")
  set(BWhite   "${Esc}[1;37m")
endif()

# set_fairmq_cmake_policies()
#
# Sets CMake policies.
macro(set_fairmq_cmake_policies)
  # Find more details to each policy with cmake --help-policy CMPXXXX
  foreach(policy
    CMP0028 # Double colon in target name means ALIAS or IMPORTED target.
    CMP0048 # The ``project()`` command manages VERSION variables.
    CMP0054 # Only interpret ``if()`` arguments as variables or keywords when unquoted.
  )
    if(POLICY ${policy})
      cmake_policy(SET ${policy} NEW)
    endif()
  endforeach()
endmacro()


find_package(Git)
# get_git_version([DEFAULT_VERSION version] [DEFAULT_DATE date] [OUTVAR_PREFIX prefix])
#
# Sets variables #prefix#_VERSION, #prefix#_GIT_VERSION, #prefix#_DATE, #prefix#_GIT_DATE
function(get_git_version)
  cmake_parse_arguments(ARGS "" "DEFAULT_VERSION;DEFAULT_DATE;OUTVAR_PREFIX" "" ${ARGN})

  if(NOT ARGS_OUTVAR_PREFIX)
    set(ARGS_OUTVAR_PREFIX PROJECT)
  endif()

  if(GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
      OUTPUT_VARIABLE ${ARGS_OUTVAR_PREFIX}_GIT_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    if(${ARGS_OUTVAR_PREFIX}_GIT_VERSION)
      # cut first two characters "v-"
      string(SUBSTRING ${${ARGS_OUTVAR_PREFIX}_GIT_VERSION} 1 -1 ${ARGS_OUTVAR_PREFIX}_GIT_VERSION)
    endif()
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd
      OUTPUT_VARIABLE ${ARGS_OUTVAR_PREFIX}_GIT_DATE
      OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  endif()

  if(NOT ${ARGS_OUTVAR_PREFIX}_GIT_VERSION)
    if(ARGS_DEFAULT_VERSION)
      set(${ARGS_OUTVAR_PREFIX}_GIT_VERSION ${ARGS_DEFAULT_VERSION})
    else()
      set(${ARGS_OUTVAR_PREFIX}_GIT_VERSION 0.0.0.0)
    endif()
  endif()

  if(NOT ${ARGS_OUTVAR_PREFIX}_GIT_DATE)
    if(ARGS_DEFAULT_DATE)
      set(${ARGS_OUTVAR_PREFIX}_GIT_DATE ${ARGS_DEFAULT_DATE})
    else()
      set(${ARGS_OUTVAR_PREFIX}_GIT_DATE "Thu Jan 1 00:00:00 1970 +0000")
    endif()
  endif()

  string(REGEX MATCH "^([^-]*)" blubb ${${ARGS_OUTVAR_PREFIX}_GIT_VERSION})

  # return values
  set(${ARGS_OUTVAR_PREFIX}_VERSION ${CMAKE_MATCH_0} PARENT_SCOPE)
  set(${ARGS_OUTVAR_PREFIX}_DATE ${${ARGS_OUTVAR_PREFIX}_GIT_DATE} PARENT_SCOPE)
  set(${ARGS_OUTVAR_PREFIX}_GIT_VERSION ${${ARGS_OUTVAR_PREFIX}_GIT_VERSION} PARENT_SCOPE)
  set(${ARGS_OUTVAR_PREFIX}_GIT_DATE ${${ARGS_OUTVAR_PREFIX}_GIT_DATE} PARENT_SCOPE)
endfunction()


# Set defaults
macro(set_fairmq_defaults)
  string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

  # Set a default build type
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  endif()

  # Handle C++ standard level
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
  elseif(${CMAKE_CXX_STANDARD} LESS 11)
    message(FATAL_ERROR "A minimum CMAKE_CXX_STANDARD of 11 is required.")
  endif()
  if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
  endif()

  # Generate compile_commands.json file (https://clang.llvm.org/docs/JSONCompilationDatabase.html)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # Define CMAKE_INSTALL_*DIR family of variables
  include(GNUInstallDirs)

  # Define install dirs
  set(PROJECT_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
  set(PROJECT_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME_LOWER})
  set(PROJECT_INSTALL_INCDIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME_LOWER})
  set(PROJECT_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME_LOWER})
  set(PROJECT_INSTALL_CMAKEMODDIR ${${PROJECT_NAME}_INSTALL_DATADIR}/cmake)

  # Define export set, only one for now
  set(PROJECT_EXPORT_SET ${PROJECT_NAME}Targets)
endmacro()

macro(generate_package_dependencies)
  set(PACKAGE_DEPENDENCIES "\
include(CMakeFindDependencyMacro)

set(${PROJECT_NAME}_PACKAGE_DEPENDENCIES ${PROJECT_PACKAGE_DEPENDENCIES})

")
  foreach(dep IN LISTS PROJECT_PACKAGE_DEPENDENCIES)
    string(CONCAT ${PACKAGE_DEPENDENCIES} "\
set(${PROJECT_NAME}_${dep}_COMPONENTS ${PROJECT_${dep}_COMPONENTS})
set(${PROJECT_NAME}_${dep}_VERSION ${PROJECT_${dep}_VERSION})
set(${PROJECT_NAME}_${dep}_FOUND ${${dep}_FOUND})

")
  endforeach()
  string(CONCAT ${PACKAGE_DEPENDENCIES} "\
if(Boost_INCLUDE_DIR) # checks for cached boost variable which indicates if Boost is already found
  set(${PROJECT_NAME}_Boost_QUIET QUIET)
endif()

foreach(dep IN LISTS ${PROJECT_NAME}_PACKAGE_DEPENDENCIES)
  if(    NOT (${PROJECT_NAME}_\${dep}_DISABLED OR ${PROJECT_NAME}_PACKAGE_DEPENDENCIES_DISABLED)
    AND (${PROJECT_NAME}_\${dep}_FOUND OR ${PROJECT_NAME}_\${dep}_REQUIRED))
    if(${PROJECT_NAME}_ADDITIONAL_\${dep}_COMPONENTS)
      list(APPEND ${PROJECT_NAME}_\${dep}_COMPONENTS \${${PROJECT_NAME}_ADDITIONAL_\${dep}_COMPONENTS})
      list(REMOVE_DUPLICATES ${PROJECT_NAME}_\${dep}_COMPONENTS)
    endif()
    if(${PROJECT_NAME}_\${dep}_COMPONENTS)
      set(components COMPONENTS \${${PROJECT_NAME}_\${dep}_COMPONENTS})
    else()
      set(components)
    endif()
    find_dependency(\${dep} \${${PROJECT_NAME}_\${dep}_VERSION} \${${PROJECT_NAME}_\${dep}_QUIET} \${components})
  endif()
endforeach()
")
endmacro()

# Configure/Install CMake package
macro(install_cmake_package)
  # Install cmake modules
  install(DIRECTORY cmake
    PATTERN "cmake/Find*.cmake"
    DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEMODDIR}
  )

  include(CMakePackageConfigHelpers)
  set(PACKAGE_INSTALL_DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}
  )
  install(EXPORT ${${PROJECT_NAME}_EXPORT_SET}
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${PACKAGE_INSTALL_DESTINATION}
    EXPORT_LINK_INTERFACE_LIBRARIES
  )
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    COMPATIBILITY AnyNewerVersion
  )
  generate_package_dependencies() # fills ${PACKAGE_DEPENDENCIES}
  configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${PACKAGE_INSTALL_DESTINATION}
    PATH_VARS CMAKE_INSTALL_PREFIX
  )
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${PACKAGE_INSTALL_DESTINATION}
  )
endmacro()
