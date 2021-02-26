 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             #
 #              GNU Lesser General Public Licence (LGPL) version 3,             #
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################

set(EX_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(FAIRMQ_BIN_DIR ${CMAKE_BINARY_DIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-builtin-devices.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-builtin-devices.sh)

# test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-ex-builtin-devices.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test-ex-builtin-devices.sh)

add_test(NAME Example.BuiltinDevices.zeromq COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-builtin-devices.sh zeromq)
set_tests_properties(Example.BuiltinDevices.zeromq PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Configured maximum number of iterations reached")

add_test(NAME Example.BuiltinDevices.shmem COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-builtin-devices.sh shmem)
set_tests_properties(Example.BuiltinDevices.shmem PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Configured maximum number of iterations reached")

add_test(NAME Example.BuiltinDevices.multipart.zeromq COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-builtin-devices.sh zeromq true 2)
set_tests_properties(Example.BuiltinDevices.multipart.zeromq PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Configured maximum number of iterations reached")

add_test(NAME Example.BuiltinDevices.multipart.shmem COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-ex-builtin-devices.sh shmem true 2)
set_tests_properties(Example.BuiltinDevices.multipart.shmem PROPERTIES TIMEOUT "30" RUN_SERIAL true PASS_REGULAR_EXPRESSION "Configured maximum number of iterations reached")

# install

# configure run script with different executable paths for build and for install directories
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})
set(FAIRMQ_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/fairmq)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fairmq-start-ex-builtin-devices.sh.in ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-builtin-devices.sh_install)

install(
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/fairmq-start-ex-builtin-devices.sh_install
  DESTINATION ${PROJECT_INSTALL_BINDIR}
  RENAME fairmq-start-ex-builtin-devices.sh
)
