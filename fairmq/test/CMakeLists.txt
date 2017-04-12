################################################################################
#    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

find_package(GTest REQUIRED)
set(PSTREAMS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/pstreams)
find_package(PStreams REQUIRED)
include(GTestHelper)

#############################
# FairMQ Testsuites/helpers #
#############################

add_testhelper(runTestDevice
    SOURCES
    helper/runTestDevice.cxx
    helper/devices/TestPollIn.cxx
    helper/devices/TestPollOut.cxx
    helper/devices/TestPub.cxx
    helper/devices/TestPull.cxx
    helper/devices/TestPush.cxx
    helper/devices/TestRep.cxx
    helper/devices/TestReq.cxx
    helper/devices/TestSub.cxx
    helper/devices/TestTransferTimeout.cxx

    LINKS FairMQ
)


set(MQ_CONFIG "${CMAKE_BINARY_DIR}/bin/testsuite_FairMQ.IOPatterns_config.json")
set(RUN_TEST_DEVICE "${CMAKE_BINARY_DIR}/bin/testhelper_runTestDevice")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/protocols/config.json.in ${MQ_CONFIG})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/protocols/runner.cxx.in ${CMAKE_CURRENT_BINARY_DIR}/protocols/runner.cxx)

add_testsuite(FairMQ.Protocols
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/protocols/runner.cxx
    protocols/_poller.cxx
    protocols/_pub_sub.cxx
    protocols/_push_pull.cxx
    protocols/_req_rep.cxx
    protocols/_transfer_timeout.cxx
    protocols/_push_pull_multipart.cxx

    LINKS PStreams FairMQ
    DEPENDS testhelper_runTestDevice
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/protocols
    TIMEOUT 30
    RUN_SERIAL ON
)

add_testsuite(FairMQ.Parts
    SOURCES
    parts/runner.cxx
    parts/_iterator_interface.cxx

    LINKS FairMQ ZeroMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/parts
    TIMEOUT 5
)

add_testsuite(FairMQ.Device
    SOURCES
    device/TestSender.h
    device/TestReceiver.h
    device/runner.cxx
    device/_multiple_devices.cxx

    LINKS FairMQ
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/device
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
    plugins/runner.cxx
    plugins/_plugin.cxx
    plugins/_plugin_manager.cxx

    LINKS FairMQ
    DEPENDS FairMQPlugin_test_dummy FairMQPlugin_test_dummy2
    TIMEOUT 10
)

add_testsuite(FairMQ.PluginsStatic
    SOURCES
    plugins/runner.cxx
    plugins/_plugin_manager_static.cxx

    LINKS FairMQ FairMQPlugin_test_dummy FairMQPlugin_test_dummy2
    TIMEOUT 10
)

##############################
# Aggregate all test targets #
##############################
add_custom_target(FairMQTests
    DEPENDS
    ${ALL_TEST_TARGETS}
)


