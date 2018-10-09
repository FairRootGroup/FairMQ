################################################################################
# Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

include(GTestHelper)

#############################
# FairMQ Testsuites/helpers #
#############################

add_testhelper(runTestDevice
    SOURCES
    helper/runTestDevice.cxx
    helper/devices/TestPairLeft.cxx
    helper/devices/TestPairRight.cxx
    helper/devices/TestPollIn.cxx
    helper/devices/TestPollOut.cxx
    helper/devices/TestPub.cxx
    helper/devices/TestPull.cxx
    helper/devices/TestPush.cxx
    helper/devices/TestRep.cxx
    helper/devices/TestReq.cxx
    helper/devices/TestSub.cxx
    helper/devices/TestTransferTimeout.cxx
    helper/devices/TestWaitFor.cxx
    helper/devices/TestExceptions.cxx

    LINKS FairMQ
)

if(BUILD_NANOMSG_TRANSPORT)
  set(definitions DEFINITIONS BUILD_NANOMSG_TRANSPORT)
endif()

set(MQ_CONFIG "${CMAKE_BINARY_DIR}/test/testsuite_FairMQ.IOPatterns_config.json")
set(RUN_TEST_DEVICE "${CMAKE_BINARY_DIR}/test/testhelper_runTestDevice")
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/protocols/config.json.in ${MQ_CONFIG})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/runner.cxx.in ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TestEnvironment.h.in ${CMAKE_CURRENT_BINARY_DIR}/TestEnvironment.h)

add_testsuite(FairMQ.Protocols
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
    TIMEOUT 30
    RUN_SERIAL ON
    ${definitions}
)

add_testsuite(FairMQ.Parts
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    parts/_iterator_interface.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/parts
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 5
)

add_testsuite(FairMQ.MessageResize
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    message_resize/_message_resize.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/message_resize
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 5
    ${definitions}
)

add_testsuite(FairMQ.Device
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    device/TestSender.h
    device/TestReceiver.h
    device/_multiple_devices.cxx
    device/_device_version.cxx
    device/_device_config.cxx
    device/_waitfor.cxx
    device/_exceptions.cxx

    LINKS FairMQ
    DEPENDS testhelper_runTestDevice
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_SOURCE_DIR}/device
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 5
    RUN_SERIAL ON
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
add_testlib(FairMQPlugin_test_dummy2
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/helper/plugins/dummy2.h
    helper/plugins/dummy2.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/helper/plugins
    HIDDEN
    VERSION ${VERSION}
)

add_testsuite(FairMQ.Plugins
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    plugins/_plugin.cxx
    plugins/_plugin_manager.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS FairMQPlugin_test_dummy FairMQPlugin_test_dummy2
    TIMEOUT 10
)

add_testsuite(FairMQ.PluginsPrelinked
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    plugins/_plugin_manager_prelink.cxx

    LINKS FairMQ FairMQPlugin_test_dummy FairMQPlugin_test_dummy2
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 10
)

add_testsuite(FairMQ.PluginServices
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    plugin_services/_config.cxx
    plugin_services/_control.cxx
    plugin_services/Fixture.h

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 10
)

add_testsuite(FairMQ.EventManager
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    event_manager/_event_manager.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 10
)

add_testsuite(FairMQ.StateMachine
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    state_machine/_state_machine.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 10
)

add_testsuite(FairMQ.Transport
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    transport/_transfer_timeout.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 10
)

add_testsuite(FairMQ.Poller
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/runner.cxx
    poller/_poller.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
             ${CMAKE_CURRENT_BINARY_DIR}
    TIMEOUT 10
)
