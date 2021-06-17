################################################################################
# Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

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
if(NOT DEFINED CMAKE_CXX_EXTENSIONS)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(NOT BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Whether to build shared libraries or static archives")
endif()

# Set -fPIC as default for all library types
if(NOT CMAKE_POSITION_INDEPENDENT_CODE)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Generate compile_commands.json file (https://clang.llvm.org/docs/JSONCompilationDatabase.html)
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

# Define install dirs
set(PROJECT_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(PROJECT_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(PROJECT_INSTALL_INCDIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME_LOWER})
set(PROJECT_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME_LOWER})
set(PROJECT_INSTALL_CMAKEMODDIR ${PROJECT_INSTALL_DATADIR}/cmake)

# https://cmake.org/Wiki/CMake_RPATH_handling
if(NOT DEFINED CMAKE_INSTALL_RPATH_USE_LINK_PATH)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
endif()
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_LIBDIR}" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")
    list(APPEND CMAKE_SHARED_LINKER_FLAGS "-Wl,--enable-new-dtags")
    list(PREPEND CMAKE_INSTALL_RPATH "$ORIGIN/../${PROJECT_INSTALL_LIBDIR}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(PREPEND CMAKE_INSTALL_RPATH "@loader_path/../${PROJECT_INSTALL_LIBDIR}")
  else()
    list(PREPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_LIBDIR}")
  endif()
endif()

# Define export set, only one for now
set(PROJECT_EXPORT_SET ${PROJECT_NAME}Targets)

# Sanitizers
set(_sanitizers "")

option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" FALSE)
if(ENABLE_SANITIZER_ADDRESS)
  list(APPEND _sanitizers "address")
endif()

option(ENABLE_SANITIZER_LEAK "Enable leak sanitizer" FALSE)
if(ENABLE_SANITIZER_LEAK)
  list(APPEND _sanitizers "leak")
endif()

option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" FALSE)
if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
  list(APPEND _sanitizers "undefined")
endif()

option(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" FALSE)
if(ENABLE_SANITIZER_THREAD)
  if("address" IN_LIST _sanitizers OR "leak" IN_LIST _sanitizers)
    message(WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled")
  else()
    list(APPEND _sanitizers "thread")
  endif()
endif()

option(ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" FALSE)
if(ENABLE_SANITIZER_MEMORY AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  if("address" IN_LIST _sanitizers
     OR "thread" IN_LIST _sanitizers
     OR "leak" IN_LIST _sanitizers)
    message(WARNING "Memory sanitizer does not work with Address, Thread and Leak sanitizer enabled")
  else()
    list(APPEND _sanitizers "memory")
  endif()
endif()

list(JOIN _sanitizers "," _sanitizers)
if(_sanitizers)
  set(_sanitizers "-fsanitize=${_sanitizers}")
endif()

# Configure build types
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" "RelWithDebInfo")
set(_warnings "-Wshadow -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g ${_warnings} ${_sanitizers}")
set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g ${_warnings} -DNDEBUG ${_sanitizers}")
unset(_warnings)
unset(_sanitizers)

if(CMAKE_GENERATOR STREQUAL "Ninja" AND
   ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9) OR
    (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)))
  # Force colored warnings in Ninja's output, if the compiler has -fdiagnostics-color support.
  # Rationale in https://github.com/ninja-build/ninja/issues/814
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
endif()

if(NOT DEFINED PROJECT_VERSION_TWEAK)
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
