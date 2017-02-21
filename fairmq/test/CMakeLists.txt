 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

configure_file(${CMAKE_SOURCE_DIR}/fairmq/test/test-fairmq-push-pull.sh.in ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-push-pull.sh)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/test/test-fairmq-pub-sub.sh.in ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-pub-sub.sh)
configure_file(${CMAKE_SOURCE_DIR}/fairmq/test/test-fairmq-req-rep.sh.in ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-req-rep.sh)

Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/fairmq
  ${CMAKE_SOURCE_DIR}/fairmq/zeromq
  ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  ${CMAKE_SOURCE_DIR}/fairmq/devices
  ${CMAKE_SOURCE_DIR}/fairmq/tools
  ${CMAKE_SOURCE_DIR}/fairmq/options
  ${CMAKE_SOURCE_DIR}/fairmq/test/
  ${CMAKE_SOURCE_DIR}/fairmq/test/push-pull
  ${CMAKE_SOURCE_DIR}/fairmq/test/pub-sub
  ${CMAKE_SOURCE_DIR}/fairmq/test/req-rep
  ${CMAKE_CURRENT_BINARY_DIR}
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
  ${ZeroMQ_INCLUDE_DIR}
)

If(NANOMSG_FOUND)
  Set(SYSTEM_INCLUDE_DIRECTORIES
    ${SYSTEM_INCLUDE_DIRECTORIES}
    ${NANOMSG_INCLUDE_DIR}
  )
EndIf(NANOMSG_FOUND)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

Set(LINK_DIRECTORIES
  ${Boost_LIBRARY_DIRS}
)

Link_Directories(${LINK_DIRECTORIES})

set(SRCS
  "push-pull/FairMQTestPush.cxx"
  "push-pull/FairMQTestPull.cxx"
  "pub-sub/FairMQTestPub.cxx"
  "pub-sub/FairMQTestSub.cxx"
  "req-rep/FairMQTestReq.cxx"
  "req-rep/FairMQTestRep.cxx"
)

set(DEPENDENCIES
  ${DEPENDENCIES}
  FairMQ
)

set(LIBRARY_NAME FairMQTest)

GENERATE_LIBRARY()

set(Exe_Names
  test-fairmq-push
  test-fairmq-pull
  test-fairmq-pub
  test-fairmq-sub
  test-fairmq-req
  test-fairmq-rep
  test-fairmq-transfer-timeout
)

set(Exe_Source
  push-pull/runTestPush.cxx
  push-pull/runTestPull.cxx
  pub-sub/runTestPub.cxx
  pub-sub/runTestSub.cxx
  req-rep/runTestReq.cxx
  req-rep/runTestRep.cxx
  runTransferTimeoutTest.cxx
)

list(LENGTH Exe_Names _length)
math(EXPR _length ${_length}-1)

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES FairMQTest)
  GENERATE_EXECUTABLE()
EndForEach(_file RANGE 0 ${_length})

add_test(NAME run_fairmq_push_pull_zmq COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-push-pull.sh zeromq)
set_tests_properties(run_fairmq_push_pull_zmq PROPERTIES TIMEOUT "30")
set_tests_properties(run_fairmq_push_pull_zmq PROPERTIES PASS_REGULAR_EXPRESSION "PUSH-PULL test successfull")

add_test(NAME run_fairmq_pub_sub_zmq COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-pub-sub.sh zeromq)
set_tests_properties(run_fairmq_pub_sub_zmq PROPERTIES TIMEOUT "30")
set_tests_properties(run_fairmq_pub_sub_zmq PROPERTIES PASS_REGULAR_EXPRESSION "PUB-SUB test successfull")
Set_Tests_Properties(run_fairmq_pub_sub_zmq PROPERTIES DEPENDS run_fairmq_push_pull_zmq)

add_test(NAME run_fairmq_req_rep_zmq COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-req-rep.sh zeromq)
set_tests_properties(run_fairmq_req_rep_zmq PROPERTIES TIMEOUT "30")
set_tests_properties(run_fairmq_req_rep_zmq PROPERTIES PASS_REGULAR_EXPRESSION "REQ-REP test successfull")
Set_Tests_Properties(run_fairmq_req_rep_zmq PROPERTIES DEPENDS run_fairmq_pub_sub_zmq)

add_test(NAME run_fairmq_transfer_timeout_zmq COMMAND ${CMAKE_BINARY_DIR}/bin/test-fairmq-transfer-timeout zeromq)
set_tests_properties(run_fairmq_transfer_timeout_zmq PROPERTIES TIMEOUT "30")
set_tests_properties(run_fairmq_transfer_timeout_zmq PROPERTIES PASS_REGULAR_EXPRESSION "Transfer timeout test successfull")
Set_Tests_Properties(run_fairmq_transfer_timeout_zmq PROPERTIES DEPENDS run_fairmq_pub_sub_zmq)

If(NANOMSG_FOUND)
  add_test(NAME run_fairmq_push_pull_nn COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-push-pull.sh nanomsg)
  set_tests_properties(run_fairmq_push_pull_nn PROPERTIES TIMEOUT "30")
  set_tests_properties(run_fairmq_push_pull_nn PROPERTIES PASS_REGULAR_EXPRESSION "PUSH-PULL test successfull")
  Set_Tests_Properties(run_fairmq_push_pull_nn PROPERTIES DEPENDS run_fairmq_transfer_timeout_zmq)

  add_test(NAME run_fairmq_pub_sub_nn COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-pub-sub.sh nanomsg)
  set_tests_properties(run_fairmq_pub_sub_nn PROPERTIES TIMEOUT "30")
  set_tests_properties(run_fairmq_pub_sub_nn PROPERTIES PASS_REGULAR_EXPRESSION "PUB-SUB test successfull")
  Set_Tests_Properties(run_fairmq_pub_sub_nn PROPERTIES DEPENDS run_fairmq_push_pull_nn)

  add_test(NAME run_fairmq_req_rep_nn COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-req-rep.sh nanomsg)
  set_tests_properties(run_fairmq_req_rep_nn PROPERTIES TIMEOUT "30")
  set_tests_properties(run_fairmq_req_rep_nn PROPERTIES PASS_REGULAR_EXPRESSION "REQ-REP test successfull")
  Set_Tests_Properties(run_fairmq_req_rep_nn PROPERTIES DEPENDS run_fairmq_pub_sub_nn)

  add_test(NAME run_fairmq_transfer_timeout_nn COMMAND ${CMAKE_BINARY_DIR}/bin/test-fairmq-transfer-timeout nanomsg)
  set_tests_properties(run_fairmq_transfer_timeout_nn PROPERTIES TIMEOUT "30")
  set_tests_properties(run_fairmq_transfer_timeout_nn PROPERTIES PASS_REGULAR_EXPRESSION "Transfer timeout test successfull")
  Set_Tests_Properties(run_fairmq_transfer_timeout_nn PROPERTIES DEPENDS run_fairmq_req_rep_nn)
EndIf(NANOMSG_FOUND)
