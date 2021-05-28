################################################################################
# Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

include(CTest)
include(GNUInstallDirs)

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)

# Set a default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(PROJECT_MIN_CXX_STANDARD 17)

# Handle C++ standard level
if(CMAKE_CXX_STANDARD AND CMAKE_CXX_STANDARD VERSION_LESS PROJECT_MIN_CXX_STANDARD)
  message(FATAL_ERROR "A minimum CMAKE_CXX_STANDARD of ${PROJECT_MIN_CXX_STANDARD} is required.")
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Whether to build shared libraries or static archives")
endif()

# Set -fPIC as default for all library types
if(NOT CMAKE_POSITION_INDEPENDENT_CODE)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Generate compile_commands.json file (https://clang.llvm.org/docs/JSONCompilationDatabase.html)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define install dirs
set(PROJECT_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(PROJECT_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(PROJECT_INSTALL_INCDIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME_LOWER})
set(PROJECT_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME_LOWER})
set(PROJECT_INSTALL_CMAKEMODDIR ${PROJECT_INSTALL_DATADIR}/cmake)

# https://cmake.org/Wiki/CMake_RPATH_handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_LIBDIR}" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-Wl,--enable-new-dtags")
    set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,--enable-new-dtags")
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${PROJECT_INSTALL_LIBDIR}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_INSTALL_RPATH "@loader_path/../${PROJECT_INSTALL_LIBDIR}")
  else()
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_LIBDIR}")
  endif()
endif()

# Define export set, only one for now
set(PROJECT_EXPORT_SET ${PROJECT_NAME}Targets)

# Configure build types
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" "RelWithDebInfo" "Nightly" "Profile" "Experimental" "AddressSan" "ThreadSan")
set(_warnings "-Wshadow -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g ${_warnings}")
set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g ${_warnings} -DNDEBUG")
set(CMAKE_CXX_FLAGS_NIGHTLY        "-O2 -g ${_warnings}")
set(CMAKE_CXX_FLAGS_PROFILE        "-g3 ${_warnings} -fno-inline -ftest-coverage -fprofile-arcs")
set(CMAKE_CXX_FLAGS_EXPERIMENTAL   "-O2 -g ${_warnings} -DNDEBUG")
set(CMAKE_CXX_FLAGS_ADDRESSSAN     "-O2 -g ${_warnings} -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_THREADSAN      "-O2 -g ${_warnings} -fsanitize=thread")
unset(_warnings)

if(CMAKE_GENERATOR STREQUAL "Ninja" AND
   ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9) OR
    (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)))
  # Force colored warnings in Ninja's output, if the compiler has -fdiagnostics-color support.
  # Rationale in https://github.com/ninja-build/ninja/issues/814
  list(APPEND CMAKE_CXX_FLAGS "-fdiagnostics-color=always")
endif()

if(NOT PROJECT_VERSION_TWEAK)
  set(PROJECT_VERSION_HOTFIX 0)
else()
  set(PROJECT_VERSION_HOTFIX ${PROJECT_VERSION_TWEAK})
endif()

if(NOT DEFINED RUN_STATIC_ANALYSIS)
  set(RUN_STATIC_ANALYSIS OFF)
endif()

unset(PROJECT_STATIC_ANALYSERS)
if(RUN_STATIC_ANALYSIS)
  set(analyser "clang-tidy")
  find_program(${analyser}_FOUND "${analyser}")
  if(${analyser}_FOUND)
    set(CMAKE_CXX_CLANG_TIDY "${${analyser}_FOUND}")
  endif()
  list(APPEND PROJECT_STATIC_ANALYSERS "${analyser}")

  set(analyser "iwyu")
  find_program(${analyser}_FOUND "${analyser}")
  if(${analyser}_FOUND)
    set(CMAKE_CXX_IWYU "${${analyser}_FOUND}")
  endif()
  list(APPEND PROJECT_STATIC_ANALYSERS "${analyser}")

  set(analyser "cpplint")
  find_program(${analyser}_FOUND "${analyser}")
  if(${analyser}_FOUND)
    set(CMAKE_CXX_CPPLINT "${${analyser}_FOUND}")
  endif()
  list(APPEND PROJECT_STATIC_ANALYSERS "${analyser}")
endif()

if(CMAKE_GENERATOR STREQUAL Ninja AND ENABLE_CCACHE)
  find_program(CCACHE ccache)
  if(CCACHE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
  endif()
endif()

if(    CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
  set(FAIRMQ_HAS_STD_FILESYSTEM 0)
else()
  set(FAIRMQ_HAS_STD_FILESYSTEM 1)
endif()
