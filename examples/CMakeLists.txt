################################################################################
# Copyright (C) 2018-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(exe_prefix "fairmq-ex")
set(script_prefix "fairmq-start-ex")
set(test_script_prefix "test-ex")
set(testsuite "Example")
set(transports "zeromq" "shmem")

# Environment for every example test (directory scope, so the subdirectories
# see it too)
set(env_mods)
if(ENABLE_SANITIZER_LEAK AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
  get_filename_component(lsan_supps "${CMAKE_SOURCE_DIR}/test/leak_sanitizer_suppressions.txt" ABSOLUTE)
  list(APPEND env_mods "LSAN_OPTIONS=set:suppressions=${lsan_supps}")
endif()
# Run the example tests against an alternative runtime library directory as
# well (see test/CMakeLists.txt). The test scripts only launch instrumented
# binaries (bash itself does not link libstdc++). Note: unlike the
# testsuites, the example binaries get no locale-cache warmup -- their
# main() comes from the installed public header fairmq/runDevice.h.
if(FAIRMQ_TEST_LD_LIBRARY_PATH)
  list(APPEND env_mods "LD_LIBRARY_PATH=path_list_prepend:${FAIRMQ_TEST_LD_LIBRARY_PATH}")
endif()

function(add_example)
  cmake_parse_arguments(PARSE_ARGV 0 ARG
    "CONFIG;NO_TRANSPORT;NO_TEST"
    "NAME"
    "DEVICE;VARIANT;TRANSPORT;SCRIPT"
  )

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unexpected unparsed arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(ARG_NAME)
    set(name ${ARG_NAME})
  else()
    message(FATAL_ERROR "NAME arg is required")
  endif()

  if(ARG_DEVICE)
    set(exe_targets)
    foreach(device IN LISTS ARG_DEVICE)
      set(exe "${exe_prefix}-${name}-${device}")
      list(APPEND exe_targets ${exe})
      add_executable(${exe} "${device}.cxx")
      target_link_libraries(${exe} PRIVATE FairMQ)
    endforeach()
  endif()

  if(ARG_TRANSPORT)
    set(transports ${ARG_TRANSPORT})
  endif()

  if(ARG_SCRIPT)
    set(scripts ${ARG_SCRIPT})
  else()
    set(scripts ${ARG_NAME})
  endif()

  set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
  if(ARG_CONFIG)
    set(EX_CONF_DIR ${CMAKE_CURRENT_BINARY_DIR})
  endif()
  set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
  foreach(script IN LISTS scripts)
    set(script_file "${script_prefix}-${script}.sh")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${script_file}.in" "${CMAKE_CURRENT_BINARY_DIR}/${script_file}" @ONLY)
  endforeach()

  if(ARG_CONFIG)
    set(config "ex-${name}.json")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${config}" "${CMAKE_CURRENT_BINARY_DIR}/${config}")
  endif()

  # test
  if(NOT ARG_NO_TEST)
    set(test_script "${test_script_prefix}-${name}.sh")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${test_script}.in" "${CMAKE_CURRENT_BINARY_DIR}/${test_script}")

    if(ARG_NO_TRANSPORT)
      set(test "${testsuite}.${name}.${transport}")
      add_test(NAME ${test} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_script} ${transport})
      set_tests_properties(${test} PROPERTIES TIMEOUT "30")
      if(env_mods)
        set_tests_properties(${test} PROPERTIES ENVIRONMENT_MODIFICATION "${env_mods}")
      endif()
    else()
      foreach(transport IN LISTS transports)
        if(ARG_VARIANT)
          foreach(variant IN LISTS ARG_VARIANT)
            set(test "${testsuite}.${name}.${variant}.${transport}")
            add_test(NAME ${test} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_script} ${transport} ${variant})
            set_tests_properties(${test} PROPERTIES TIMEOUT "30")
            if(env_mods)
              set_tests_properties(${test} PROPERTIES ENVIRONMENT_MODIFICATION "${env_mods}")
            endif()
          endforeach()
        else()
          set(test "${testsuite}.${name}.${transport}")
          add_test(NAME ${test} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_script} ${transport})
          set_tests_properties(${test} PROPERTIES TIMEOUT "30")
          if(env_mods)
            set_tests_properties(${test} PROPERTIES ENVIRONMENT_MODIFICATION "${env_mods}")
          endif()
        endif()
      endforeach()
    endif()
  endif()

  # install
  install(
    TARGETS ${exe_targets}
    LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
  )

  # configure run script with different executable paths for build and for install directories
  set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
  if(ARG_CONFIG)
    set(EX_CONF_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_DATADIR})
  endif()
  set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
  foreach(script IN LISTS scripts)
    set(script_file "${script_prefix}-${script}.sh")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${script_file}.in" "${CMAKE_CURRENT_BINARY_DIR}/${script_file}_install" @ONLY)
    install(
      PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${script_file}_install"
      DESTINATION ${PROJECT_INSTALL_BINDIR}
      RENAME ${script_file}
    )
  endforeach()

  if(ARG_CONFIG)
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/${config}
      DESTINATION ${PROJECT_INSTALL_DATADIR}
    )
  endif()
endfunction()

add_subdirectory(1-1)
add_subdirectory(1-n-1)
add_subdirectory(builtin-devices)
add_subdirectory(copypush)
add_subdirectory(custom-controller)
add_subdirectory(dds)
add_subdirectory(multipart)
add_subdirectory(multiple-channels)
add_subdirectory(multiple-transports)
add_subdirectory(n-m)
add_subdirectory(qc)
add_subdirectory(readout)
add_subdirectory(region)
add_subdirectory(req-rep)
