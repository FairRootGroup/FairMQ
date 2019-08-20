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
  string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)

  # Set a default build type
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  endif()

  # Handle C++ standard level
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD ${PROJECT_MIN_CXX_STANDARD})
  elseif(${CMAKE_CXX_STANDARD} LESS ${PROJECT_MIN_CXX_STANDARD})
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

  # Define CMAKE_INSTALL_*DIR family of variables
  include(GNUInstallDirs)

  # Define install dirs
  set(PROJECT_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
  set(PROJECT_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
  set(PROJECT_INSTALL_INCDIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME_LOWER})
  set(PROJECT_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME_LOWER})
  set(PROJECT_INSTALL_CMAKEMODDIR ${PROJECT_INSTALL_DATADIR}/cmake)

  # https://cmake.org/Wiki/CMake_RPATH_handling
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
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
  set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" "RelWithDebInfo" "Nightly" "Profile" "Experimental" "AdressSan" "ThreadSan")
  set(CMAKE_CXX_FLAGS_DEBUG          "-g -Wshadow -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wshadow -Wall -Wextra -DNDEBUG")
  set(CMAKE_CXX_FLAGS_NIGHTLY        "-O2 -g -Wshadow -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_PROFILE        "-g3 -Wshadow -Wall -Wextra -fno-inline -ftest-coverage -fprofile-arcs")
  set(CMAKE_CXX_FLAGS_EXPERIMENTAL   "-O2 -g -Wshadow -Wall -Wextra -DNDEBUG")
  set(CMAKE_CXX_FLAGS_ADRESSSAN      "-O2 -g -Wshadow -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS_THREADSAN      "-O2 -g -Wshadow -Wall -Wextra -fsanitize=thread")

  if(CMAKE_GENERATOR STREQUAL "Ninja" AND
     ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9) OR
      (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)))
    # Force colored warnings in Ninja's output, if the compiler has -fdiagnostics-color support.
    # Rationale in https://github.com/ninja-build/ninja/issues/814
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
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
endmacro()

function(join VALUES GLUE OUTPUT)
  string(REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
  string(REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
  set(${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

function(pad str width char out)
  cmake_parse_arguments(ARGS "" "COLOR" "" ${ARGN})
  string(LENGTH ${str} length)
  if(ARGS_COLOR)
    math(EXPR padding "${width}-(${length}-10*${ARGS_COLOR})")
  else()
    math(EXPR padding "${width}-${length}")
  endif()
  if(padding GREATER 0)
    foreach(i RANGE ${padding})
      set(str "${str}${char}")
    endforeach()
  endif()
  set(${out} ${str} PARENT_SCOPE)
endfunction()

function(generate_package_dependencies)
  join("${PROJECT_INTERFACE_PACKAGE_DEPENDENCIES}" " " DEPS)
  set(PACKAGE_DEPENDENCIES "\
####### Expanded from @PACKAGE_DEPENDENCIES@ by configure_package_config_file() #######

set(${PROJECT_NAME}_PACKAGE_DEPENDENCIES ${DEPS})

")
  foreach(dep IN LISTS PROJECT_INTERFACE_PACKAGE_DEPENDENCIES)
    join("${PROJECT_INTERFACE_${dep}_COMPONENTS}" " " COMPS)
    if(COMPS)
      string(CONCAT PACKAGE_DEPENDENCIES ${PACKAGE_DEPENDENCIES} "\
set(${PROJECT_NAME}_${dep}_COMPONENTS ${COMPS})
")
    endif()
    if(PROJECT_INTERFACE_${dep}_VERSION)
      string(CONCAT PACKAGE_DEPENDENCIES ${PACKAGE_DEPENDENCIES} "\
set(${PROJECT_NAME}_${dep}_VERSION ${PROJECT_INTERFACE_${dep}_VERSION})
")
    endif()
  endforeach()
  string(CONCAT PACKAGE_DEPENDENCIES ${PACKAGE_DEPENDENCIES} "\

#######################################################################################
")
set(PACKAGE_DEPENDENCIES ${PACKAGE_DEPENDENCIES} PARENT_SCOPE)
endfunction()

function(generate_package_components)
  join("${PROJECT_PACKAGE_COMPONENTS}" " " COMPS)
  set(PACKAGE_COMPONENTS "\
####### Expanded from @PACKAGE_COMPONENTS@ by configure_package_config_file() #########

set(${PROJECT_NAME}_PACKAGE_COMPONENTS ${COMPS})

")
  foreach(comp IN LISTS PROJECT_PACKAGE_COMPONENTS)
    string(CONCAT PACKAGE_COMPONENTS ${PACKAGE_COMPONENTS} "\
set(${PROJECT_NAME}_${comp}_FOUND TRUE)
")
  endforeach()
  string(CONCAT PACKAGE_COMPONENTS ${PACKAGE_COMPONENTS} "\

check_required_components(${PROJECT_NAME})
")
set(PACKAGE_COMPONENTS ${PACKAGE_COMPONENTS} PARENT_SCOPE)
endfunction()

function(generate_bundled_packages)
  if(asio_BUNDLED)
  set(BUNDLED_PACKAGES "\
####### Expanded from @BUNDLED_PACKAGES@ by configure_package_config_file() #########

if(\"\${CMAKE_MAJOR_VERSION}.\${CMAKE_MINOR_VERSION}\" VERSION_LESS 3.11)
   message(FATAL_ERROR \"CMake >= 3.11 required\")
endif()
set_target_properties(${PROJECT_NAME}::bundled_asio_headers PROPERTIES IMPORTED_GLOBAL TRUE)
add_library(asio::headers ALIAS ${PROJECT_NAME}::bundled_asio_headers)
set(asio_VERSION ${asio_VERSION})

")
  endif()
set(BUNDLED_PACKAGES ${BUNDLED_PACKAGES} PARENT_SCOPE)
endfunction()

# Configure/Install CMake package
macro(install_cmake_package)
  list(SORT PROJECT_PACKAGE_DEPENDENCIES)
  list(SORT PROJECT_INTERFACE_PACKAGE_DEPENDENCIES)
  include(CMakePackageConfigHelpers)
  set(PACKAGE_INSTALL_DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}
  )
  if(BUILD_FAIRMQ)
    install(EXPORT ${PROJECT_EXPORT_SET}
      NAMESPACE ${PROJECT_NAME}::
      DESTINATION ${PACKAGE_INSTALL_DESTINATION}
      EXPORT_LINK_INTERFACE_LIBRARIES
    )
  endif()
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
  )
  generate_package_dependencies() # fills ${PACKAGE_DEPENDENCIES}
  generate_package_components() # fills ${PACKAGE_COMPONENTS}
  generate_bundled_packages() # fills ${BUNDLED_PACKAGES}
  string(TOUPPER ${CMAKE_BUILD_TYPE} PROJECT_BUILD_TYPE_UPPER)
  set(PROJECT_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${PROJECT_BUILD_TYPE_UPPER}})
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

#
# find_package2(PRIVATE|PUBLIC|INTERFACE <pkgname>
#               [VERSION <version>]
#               [COMPONENTS <list of components>]
#               [ADD_REQUIREMENTS_OF <list of dep_pgkname>]
#               [any other option the native find_package supports]...)
#
# Wrapper around CMake's native find_package command to add some features and bookkeeping.
#
# The qualifier (PRIVATE|PUBLIC|INTERFACE|BUNDLED) to the package to populate
# the variables PROJECT_[INTERFACE]_<pkgname>_([VERSION]|[COMPONENTS]|PACKAGE_DEPENDENCIES)
# accordingly. This bookkeeping information is used to print our dependency found summary
# table and to generate a part of our CMake package. BUNDLED decays to PUBLIC if the variable
# <pkgname>_BUNDLED is false and to PRIVATE otherwise.
#
# When a dependending package is listed with ADD_REQUIREMENTS_OF the variables
# <dep_pkgname>_<pkgname>_VERSION|COMPONENTS are looked up to and added to the native
# VERSION (selected highest version) and COMPONENTS (deduplicated) args.
#
# COMPONENTS and VERSION args are then just passed to the native find_package.
#
macro(find_package2 qualifier pkgname)
  cmake_parse_arguments(ARGS "" "VERSION" "COMPONENTS;ADD_REQUIREMENTS_OF" ${ARGN})

  string(TOUPPER ${pkgname} pkgname_upper)
  set(__old_cpp__ ${CMAKE_PREFIX_PATH})
  set(CMAKE_PREFIX_PATH ${${pkgname_upper}_ROOT} $ENV{${pkgname_upper}_ROOT} ${CMAKE_PREFIX_PATH})

  # build lists of required versions and components
  unset(__required_versions__)
  unset(__components__)
  if(ARGS_VERSION)
    list(APPEND __required_versions__ ${ARGS_VERSION})
  endif()
  if(ARGS_COMPONENTS)
    list(APPEND __components__ ${ARGS_COMPONENTS})
  endif()
  if(ARGS_ADD_REQUIREMENTS_OF)
    foreach(dep_pkgname IN LISTS ARGS_ADD_REQUIREMENTS_OF)
      if(${dep_pkgname}_${pkgname}_VERSION)
        list(APPEND __required_versions__ ${${dep_pkgname}_${pkgname}_VERSION})
      endif()
      if(${dep_pkgname}_${pkgname}_COMPONENTS)
        list(APPEND __components__ ${${dep_pkgname}_${pkgname}_COMPONENTS})
      endif()
    endforeach()
  endif()

  # select highest required version
  unset(__version__)
  if(__required_versions__)
    list(GET __required_versions__ 0 __version__)
    foreach(v IN LISTS __required_versions__)
      if(${v} VERSION_GREATER ${__version__})
        set(__version__ ${v})
      endif()
    endforeach()
  endif()
  # deduplicate required component list
  if(__components__)
    list(REMOVE_DUPLICATES __components__)
  endif()

  # call native find_package
  if(__components__)
    find_package(${pkgname} ${__version__} QUIET COMPONENTS ${__components__} ${ARGS_UNPARSED_ARGUMENTS})
  else()
    find_package(${pkgname} ${__version__} QUIET ${ARGS_UNPARSED_ARGUMENTS})
  endif()

  if(${qualifier} STREQUAL BUNDLED)
    if(${pkgname}_BUNDLED)
      set(__qualifier__ PRIVATE)
    else()
      set(__qualifier__ PUBLIC)
    endif()
  else()
    set(__qualifier__ ${qualifier})
  endif()

  if(${pkgname}_FOUND)
    if(${__qualifier__} STREQUAL PRIVATE)
      set(PROJECT_${pkgname}_VERSION ${__version__})
      set(PROJECT_${pkgname}_COMPONENTS ${__components__})
      set(PROJECT_PACKAGE_DEPENDENCIES ${PROJECT_PACKAGE_DEPENDENCIES} ${pkgname})
    elseif(${__qualifier__} STREQUAL PUBLIC)
      set(PROJECT_${pkgname}_VERSION ${__version__})
      set(PROJECT_${pkgname}_COMPONENTS ${__components__})
      set(PROJECT_PACKAGE_DEPENDENCIES ${PROJECT_PACKAGE_DEPENDENCIES} ${pkgname})
      set(PROJECT_INTERFACE_${pkgname}_VERSION ${__version__})
      set(PROJECT_INTERFACE_${pkgname}_COMPONENTS ${__components__})
      set(PROJECT_INTERFACE_PACKAGE_DEPENDENCIES ${PROJECT_INTERFACE_PACKAGE_DEPENDENCIES} ${pkgname})
    elseif(${__qualifier__} STREQUAL INTERFACE)
      set(PROJECT_INTERFACE_${pkgname}_VERSION ${__version__})
      set(PROJECT_INTERFACE_${pkgname}_COMPONENTS ${__components__})
      set(PROJECT_INTERFACE_PACKAGE_DEPENDENCIES ${PROJECT_INTERFACE_PACKAGE_DEPENDENCIES} ${pkgname})
    endif()
  endif()

  unset(__qualifier__)
  unset(__version__)
  unset(__components__)
  unset(__required_versions__)
  set(CMAKE_PREFIX_PATH ${__old_cpp__})
  unset(__old_cpp__)
endmacro()

macro(exec cmd)
  join("${ARGN}" " " args)
  file(APPEND ${${package}_BUILD_LOGFILE} ">>> ${cmd} ${args}\n")

  execute_process(COMMAND ${cmd} ${ARGN}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    OUTPUT_VARIABLE log
    ERROR_VARIABLE log
    RESULT_VARIABLE res
  )
  file(APPEND ${${package}_BUILD_LOGFILE} ${log})

  if(res)
    message(FATAL_ERROR "${res} \nSee also \"${${package}_BUILD_LOGFILE}\"")
  endif()
endmacro()

function(build_bundled package bundle)
  message(STATUS "Building bundled ${package}")

  set(${package}_SOURCE_DIR ${CMAKE_SOURCE_DIR}/${bundle})
  set(${package}_BINARY_DIR ${CMAKE_BINARY_DIR}/${bundle})
  file(MAKE_DIRECTORY ${${package}_BINARY_DIR})
  set(${package}_BUILD_LOGFILE ${${package}_BINARY_DIR}/build.log)
  file(REMOVE ${${package}_BUILD_LOGFILE})

  if(Git_FOUND)
    exec(${GIT_EXECUTABLE} submodule update --init --recursive -- ${${package}_SOURCE_DIR})
  endif()

  if(${package} STREQUAL GTest)
    set(${package}_INSTALL_DIR ${CMAKE_BINARY_DIR}/${bundle}_install)
    file(MAKE_DIRECTORY ${${package}_INSTALL_DIR})
    set(${package}_ROOT ${${package}_INSTALL_DIR})

    exec(${CMAKE_COMMAND} -S ${${package}_SOURCE_DIR} -B ${${package}_BINARY_DIR} -G ${CMAKE_GENERATOR}
      -DCMAKE_INSTALL_PREFIX=${${package}_INSTALL_DIR} -DBUILD_GMOCK=OFF
    )
    exec(${CMAKE_COMMAND} --build ${${package}_BINARY_DIR})
    exec(${CMAKE_COMMAND} --build ${${package}_BINARY_DIR} --target install)
  elseif(${package} STREQUAL asio)
    set(${package}_BUILD_INCLUDE_DIR ${${package}_SOURCE_DIR}/asio/include CACHE PATH "Bundled ${package} build-interface include dir")
    set(${package}_INSTALL_INCLUDE_DIR ${PROJECT_INSTALL_INCDIR}/bundled CACHE PATH "Bundled ${package} install-interface include dir")
    set(${package}_ROOT ${${package}_SOURCE_DIR}/asio)
  endif()

  string(TOUPPER ${package} package_upper)
  set(${package_upper}_ROOT "${${package}_ROOT}" CACHE PATH "Bundled ${package} install prefix search hint")
  set(${package}_BUNDLED ON CACHE BOOL "Whether bundled ${package} was used")

  message(STATUS "Building bundled ${package} - done")
endfunction()
