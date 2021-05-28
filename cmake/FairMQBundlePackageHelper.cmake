################################################################################
# Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include_guard(GLOBAL)

if(NOT DEFINED ${GIT_EXECUTABLE})
  find_program(GIT_EXECUTABLE NAMES git)
endif()

# TODO Deduplicate code

macro(exec cmd)
  list(JOIN ARGN " " args)
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

macro(exec_source cmd)
  list(JOIN ARGN " " args)
  file(APPEND ${${package}_BUILD_LOGFILE} ">>> ${cmd} ${args}\n")

  execute_process(COMMAND ${cmd} ${ARGN}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
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

  if(GIT_EXECUTABLE)
    exec_source(${GIT_EXECUTABLE} submodule update --init -- ${${package}_SOURCE_DIR})
  endif()

  if(${package} STREQUAL GTest)
    set(${package}_INSTALL_DIR ${CMAKE_BINARY_DIR}/${bundle}_install)
    file(MAKE_DIRECTORY ${${package}_INSTALL_DIR})
    set(${package}_PREFIX ${${package}_INSTALL_DIR})

    exec(${CMAKE_COMMAND} -S ${${package}_SOURCE_DIR} -B ${${package}_BINARY_DIR} -G ${CMAKE_GENERATOR}
      -DCMAKE_INSTALL_PREFIX=${${package}_INSTALL_DIR} -DBUILD_GMOCK=OFF
    )
    exec(${CMAKE_COMMAND} --build ${${package}_BINARY_DIR})
    exec(${CMAKE_COMMAND} --build ${${package}_BINARY_DIR} --target install)
  elseif(${package} STREQUAL asio)
    set(${package}_INSTALL_DIR ${CMAKE_BINARY_DIR}/${bundle}_install)
    file(MAKE_DIRECTORY ${${package}_INSTALL_DIR})
    set(${package}_PREFIX ${${package}_INSTALL_DIR})

    exec(${CMAKE_COMMAND} -S ${${package}_SOURCE_DIR} -B ${${package}_BINARY_DIR} -G ${CMAKE_GENERATOR}
      -DCMAKE_INSTALL_PREFIX=${${package}_INSTALL_DIR}
    )
    exec(${CMAKE_COMMAND} --build ${${package}_BINARY_DIR})
    exec(${CMAKE_COMMAND} --build ${${package}_BINARY_DIR} --target install)
  elseif(${package} STREQUAL FairCMakeModules)
    set(${package}_INSTALL_DIR ${CMAKE_BINARY_DIR}/${bundle}_install)
    file(MAKE_DIRECTORY ${${package}_INSTALL_DIR})
    set(${package}_PREFIX ${${package}_INSTALL_DIR})

    exec(${CMAKE_COMMAND} -S ${${package}_SOURCE_DIR} -B ${${package}_BINARY_DIR} -G ${CMAKE_GENERATOR}
      -DCMAKE_INSTALL_PREFIX=${${package}_INSTALL_DIR}
    )
    exec(${CMAKE_COMMAND} --build ${${package}_BINARY_DIR})
    exec(${CMAKE_COMMAND} --build ${${package}_BINARY_DIR} --target install)
  elseif(${package} STREQUAL PicoSHA2)
    set(${package}_PREFIX ${${package}_SOURCE_DIR})
  endif()

  set(${package}_ROOT ${${package}_PREFIX} CACHE PATH "Location of bundle package ${package}")
  set(${package}_BUNDLED ON CACHE BOOL "Whether bundled ${package} was used")

  message(STATUS "Building bundled ${package} - done")
endfunction()

# TODO Eventually retire supporting PUBLIC bundled packages.
macro(install_bundled package)
  if(${package} STREQUAL asio)
    add_library(bundled_asio_headers INTERFACE)
    target_include_directories(bundled_asio_headers INTERFACE
      $<BUILD_INTERFACE:${asio_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}>
      $<INSTALL_INTERFACE:${PROJECT_INSTALL_INCDIR}/bundled>
    )
    install(TARGETS bundled_asio_headers EXPORT ${PROJECT_EXPORT_SET})
    install(DIRECTORY "${asio_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}/asio"
      DESTINATION ${PROJECT_INSTALL_INCDIR}/bundled
    )
    install(FILES "${asio_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}/asio.hpp"
      DESTINATION ${PROJECT_INSTALL_INCDIR}/bundled
    )
  endif()
endmacro()

# TODO Eventually retire supporting PUBLIC bundled packages.
function(fairmq_generate_bundled_packages)
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
