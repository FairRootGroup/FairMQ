################################################################################
# Copyright (C) 2017-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

# ##########################
# # GTest helper functions #
# ##########################
#
# The helper functions allow concise cmake files for GTest based test submodules.
# Testsuites register themselves automatically as CTest test.
#
#
# Usage:
#
#   add_testsuite(<name> SOURCES source1 [source2 ...]
#                        [DEPENDS dep1 [dep2 ...]]
#                        [LINKS linklib1 [linklib2 ...]
#                        [INCLUDES dir1 [dir2 ...]
#                        [ENVIRONMENT var1=op:val1[;var2=op:val2 ...]]
#                        [TIMEOUT seconds]
#                        [RUN_SERIAL ON/OFF])
#
#   -> created target: testsuite_<name>
#
#   add_testhelper(<name> SOURCES source1 [source2 ...]
#                         [DEPENDS dep1 [dep2 ...]]
#                         [LINKS linklib1 [linklib2 ...]
#                         [INCLUDES dir1 [dir2 ...])
#
#   -> created target: testhelper_<name>
#
#   add_testlib(<name> SOURCES source1 [source2 ...]
#                      [DEPENDS dep1 [dep2 ...]]
#                      [LINKS linklib1 [linklib2 ...]
#                      [INCLUDES dir1 [dir2 ...])
#
#   -> created target: <name>
#
#   The above add_* functions add all created targets to the cmake
#   variable ALL_TEST_TARGETS which can be used to create an aggregate
#   target, e.g.:
#
#   add_custom_target(AllTests DEPENDS ${ALL_TEST_TARGETS})
#
#

include(GoogleTest)
if(BUILD_TIDY_TOOL)
  include(FairMQTidy)
endif()

function(add_testsuite suitename)
    cmake_parse_arguments(testsuite
        ""
        "TIMEOUT;RUN_SERIAL"
        "SOURCES;LINKS;DEPENDS;INCLUDES;DEFINITIONS;ENVIRONMENT"
        ${ARGN}
    )

    list(INSERT testsuite_LINKS 0 GTest::Main GTest::GTest)
    set(target "testsuite_${suitename}")

    add_executable(${target} ${testsuite_SOURCES})
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set_target_properties("${target}" PROPERTIES LINK_FLAGS "-Wl,--no-as-needed")
    endif()
    target_link_libraries(${target} ${testsuite_LINKS})
    if(testsuite_DEPENDS)
        add_dependencies(${target} ${testsuite_DEPENDS})
    endif()
    if(testsuite_INCLUDES)
        target_include_directories(${target} PUBLIC ${testsuite_INCLUDES})
    endif()
    if(testsuite_DEFINITIONS)
        target_compile_definitions("${target}" PUBLIC ${testsuite_DEFINITIONS})
    endif()
    if(testsuite_ENVIRONMENT AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
        set(env "ENVIRONMENT_MODIFICATION" ${testsuite_ENVIRONMENT})
    endif()
    if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
      fairmq_target_tidy(TARGET ${target})
    endif()

    # add_test(NAME "${suitename}" WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND ${target})
    if(testsuite_TIMEOUT)
        # set_tests_properties("${suitename}" PROPERTIES TIMEOUT ${testsuite_TIMEOUT})
    else()
        set(testsuite_TIMEOUT 10)
    endif()
    if(testsuite_RUN_SERIAL)
        # set_tests_properties("${suitename}" PROPERTIES RUN_SERIAL ${testsuite_RUN_SERIAL})
    else()
        set(testsuite_RUN_SERIAL OFF)
    endif()
    gtest_discover_tests(${target}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      TEST_PREFIX ${suitename}.
      PROPERTIES RUN_SERIAL ${testsuite_RUN_SERIAL}
                 TIMEOUT ${testsuite_TIMEOUT}
                 ${env}
    )

    list(APPEND ALL_TEST_TARGETS ${target})
    set(ALL_TEST_TARGETS ${ALL_TEST_TARGETS} PARENT_SCOPE)
endfunction()

function(add_testhelper helpername)
    cmake_parse_arguments(testhelper
        ""
        ""
        "SOURCES;LINKS;DEPENDS;INCLUDES;DEFINITIONS"
        ${ARGN}
    )

    set(target "testhelper_${helpername}")

    add_executable(${target} ${testhelper_SOURCES})
    if(testhelper_LINKS)
        target_link_libraries(${target} ${testhelper_LINKS})
    endif()
    if(testhelper_DEPENDS)
        add_dependencies(${target} ${testhelper_DEPENDS})
    endif()
    if(testhelper_INCLUDES)
        target_include_directories(${target} PUBLIC ${testhelper_INCLUDES})
    endif()
    if(testhelper_DEFINITIONS)
        target_compile_definitions(${target} PUBLIC ${testhelper_DEFINITIONS})
    endif()
    if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
      fairmq_target_tidy(TARGET ${target})
    endif()

    list(APPEND ALL_TEST_TARGETS ${target})
    set(ALL_TEST_TARGETS ${ALL_TEST_TARGETS} PARENT_SCOPE)
endfunction()

function(add_testlib libname)
    cmake_parse_arguments(testlib
        "HIDDEN"
        "VERSION"
        "SOURCES;LINKS;DEPENDS;INCLUDES;DEFINITIONS"
        ${ARGN}
    )

    set(target "${libname}")

    add_library(${target} SHARED ${testlib_SOURCES})
    if(testlib_LINKS)
        target_link_libraries(${target} ${testlib_LINKS})
    endif()
    if(testlib_DEPENDS)
        add_dependencies(${target} ${testlib_DEPENDS})
    endif()
    if(testlib_INCLUDES)
        target_include_directories(${target} PUBLIC ${testlib_INCLUDES})
    endif()
    if(testlib_HIDDEN)
        set_target_properties(${target} PROPERTIES CXX_VISIBILITY_PRESET hidden)
    endif()
    if(testlib_VERSION)
        set_target_properties(${target} PROPERTIES VERSION ${testlib_VERSION})
    endif()
    if(testlib_DEFINITIONS)
        target_compile_definitions(${target} PUBLIC ${testlib_DEFINITIONS})
    endif()
    if(BUILD_TIDY_TOOL AND RUN_FAIRMQ_TIDY)
      fairmq_target_tidy(TARGET ${target})
    endif()

    list(APPEND ALL_TEST_TARGETS ${target})
    set(ALL_TEST_TARGETS ${ALL_TEST_TARGETS} PARENT_SCOPE)
endfunction()
