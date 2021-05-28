################################################################################
#    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

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

  set(PROJECT_MIN_CXX_STANDARD 17)

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
endmacro()

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
add_library(asio::asio ALIAS ${PROJECT_NAME}::bundled_asio_headers)
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
  fair_generate_package_dependencies() # fills ${PACKAGE_DEPENDENCIES}
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



macro(fairmq_build_option option description)
  cmake_parse_arguments(ARGS "" "DEFAULT" "REQUIRES" ${ARGN})

  if(ARGS_DEFAULT)
    set(__default__ ON)
  else()
    set(__default__ OFF)
  endif()

  if(ARGS_REQUIRES)
    include(CMakeDependentOption)
    set(__requires__ ${ARGS_REQUIRES})
    foreach(d ${__requires__})
      string(REGEX REPLACE " +" ";" __requires_condition__ "${d}")
      if(${__requires_condition__})
      else()
        if(${option})
          message(FATAL_ERROR "Cannot enable build option ${option}, depending option is not set: ${__requires_condition__}")
        endif()
      endif()
    endforeach()
  else()
    set(__requires__)
  endif()

  if(__requires__)
    cmake_dependent_option(${option} ${description} ${__default__} "${__requires__}" OFF)
  else()
    option(${option} ${description} ${__default__})
  endif()

  set(__default__)
  set(__requires__)
  set(__requires_condition__)
  set(ARGS_DEFAULT)
  set(ARGS_REQUIRES)
  set(option)
  set(description)
endmacro()


macro(set_package_infos)
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
endmacro()
