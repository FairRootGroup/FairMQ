################################################################################
# Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include(GTestHelper)

#############################
# FairMQ Testsuites/helpers #
#############################

if(FairLogger_VERSION VERSION_LESS 1.9.0 AND FairLogger_VERSION VERSION_GREATER_EQUAL 1.7.0)
  LIST(APPEND definitions FAIR_MIN_SEVERITY=trace)
endif()

if(definitions)
  set(definitions DEFINITIONS ${definitions})
endif()

if(ENABLE_SANITIZER_LEAK)
  get_filename_component(lsan_supps "${CMAKE_CURRENT_SOURCE_DIR}/leak_sanitizer_suppressions.txt" ABSOLUTE)
  set(environment ENVIRONMENT "LSAN_OPTIONS=set:suppressions=${lsan_supps}")
endif()

add_testhelper(runTestDevice
    SOURCES
    helper/runTestDevice.cxx
    helper/devices/TestPairLeft.h
    helper/devices/TestPairRight.h
    helper/devices/TestPollIn.h
    helper/devices/TestPollOut.h
    helper/devices/TestPub.h
    helper/devices/TestPull.h
    helper/devices/TestPush.h
    helper/devices/TestRep.h
    helper/devices/TestReq.h
    helper/devices/TestSub.h
    helper/devices/TestTransferTimeout.h
    helper/devices/TestWaitFor.h
    helper/devices/TestExceptions.h

    LINKS FairMQ
    ${definitions}
)

set(RUN_TEST_DEVICE "${CMAKE_BINARY_DIR}/test/testhelper_runTestDevice")
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
set(SDK_TESTSUITE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/runner.cxx.in ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TestEnvironment.h.in ${CMAKE_CURRENT_BINARY_DIR}/TestEnvironment.h)

add_testsuite(Protocols
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    protocols/_pair.cxx
    protocols/_pub_sub.cxx
    protocols/_push_pull.cxx
    protocols/_req_rep.cxx
    protocols/_push_pull_multipart.cxx

    LINKS FairMQ
    DEPENDS testhelper_runTestDevice
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/protocols
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${definitions}
    ${environment}
)

add_testsuite(Parts
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    parts/_add_part.cxx
    parts/_iterator_interface.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/parts
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${environment}
)

add_testsuite(Message
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    message/_message.cxx

    LINKS FairMQ PicoSHA2
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/message
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${definitions}
    ${environment}
)

add_testsuite(Region
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    region/_creation.cxx
    region/_region.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/region
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${definitions}
    ${environment}
)

add_testsuite(Device
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    device/TestSender.h
    device/TestReceiver.h
    device/_multiple_devices.cxx
    device/_version.cxx
    device/_config.cxx
    device/_waitfor.cxx
    device/_exceptions.cxx
    device/_error_state.cxx
    device/_signals.cxx
    device/_transitions.cxx

    LINKS FairMQ
    DEPENDS testhelper_runTestDevice
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/device
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${environment}
)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/helper/plugins/dummy.h.in ${CMAKE_CURRENT_BINARY_DIR}/helper/plugins/dummy.h)
add_testlib(FairMQPlugin_test_dummy
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/helper/plugins/dummy.h
    helper/plugins/dummy.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/helper/plugins
    HIDDEN
    VERSION ${VERSION}
)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/helper/plugins/dummy2.h.in ${CMAKE_CURRENT_BINARY_DIR}/helper/plugins/dummy2.h)
add_testlib(fairmq-plugin-test_dummy2
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/helper/plugins/dummy2.h
    helper/plugins/dummy2.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/helper/plugins
    HIDDEN
    VERSION ${VERSION}
)

add_testsuite(Plugins
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    plugins/_plugin.cxx
    plugins/_plugin_manager.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS FairMQPlugin_test_dummy fairmq-plugin-test_dummy2
    TIMEOUT 20
    ${environment}
)

add_testsuite(PluginsPrelinked
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    plugins/_plugin_manager_prelink.cxx

    LINKS FairMQ FairMQPlugin_test_dummy fairmq-plugin-test_dummy2
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${environment}
)

add_testsuite(PluginServices
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    plugin_services/_config.cxx
    plugin_services/_control.cxx
    plugin_services/Fixture.h

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${environment}
)

add_testsuite(EventManager
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    event_manager/_event_manager.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${environment}
)

add_testsuite(Properties
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    properties/_properties.cxx
    properties/_suboptparser.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${environment}
)

# add_testsuite(StateMachine
#     SOURCES
#     ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
#     state_machine/_state_machine.cxx

    # LINKS FairMQ
    # INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             # ${CMAKE_CURRENT_BINARY_DIR}
    # TIMEOUT 5
# )

add_testsuite(Tools
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    tools/_network.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${environment}
)

add_testsuite(Channel
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    channel/_channel.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${environment}
)

add_testsuite(Transport
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    transport/_transfer_timeout.cxx
    transport/_options.cxx
    transport/_shmem.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${definitions}
    ${environment}
)

add_testsuite(Poller
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    poller/_poller.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${definitions}
    ${environment}
)

add_testsuite(MemoryResources
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    memory_resources/_memory_resources.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 20
    ${definitions}
    ${environment}
)
