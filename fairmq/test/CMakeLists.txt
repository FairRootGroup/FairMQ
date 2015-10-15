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
)

If(NANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/nanomsg
  )
Else(NANOMSG_FOUND)
  Set(INCLUDE_DIRECTORIES
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_SOURCE_DIR}/fairmq/zeromq
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

add_test(NAME run_fairmq_push_pull COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-push-pull.sh)
set_tests_properties(run_fairmq_push_pull PROPERTIES TIMEOUT "30")
set_tests_properties(run_fairmq_push_pull PROPERTIES PASS_REGULAR_EXPRESSION "PUSH-PULL test successfull")

add_test(NAME run_fairmq_pub_sub COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-pub-sub.sh)
set_tests_properties(run_fairmq_pub_sub PROPERTIES TIMEOUT "30")
set_tests_properties(run_fairmq_pub_sub PROPERTIES PASS_REGULAR_EXPRESSION "PUB-SUB test successfull")

add_test(NAME run_fairmq_req_rep COMMAND ${CMAKE_BINARY_DIR}/fairmq/test/test-fairmq-req-rep.sh)
set_tests_properties(run_fairmq_req_rep PROPERTIES TIMEOUT "30")
set_tests_properties(run_fairmq_req_rep PROPERTIES PASS_REGULAR_EXPRESSION "REQ-REP test successfull")

add_test(NAME run_fairmq_transfer_timeout COMMAND ${CMAKE_BINARY_DIR}/bin/test-fairmq-transfer-timeout)
set_tests_properties(run_fairmq_transfer_timeout PROPERTIES TIMEOUT "30")
set_tests_properties(run_fairmq_transfer_timeout PROPERTIES PASS_REGULAR_EXPRESSION "Transfer timeout test successfull")
