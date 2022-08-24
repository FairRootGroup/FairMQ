################################################################################
#    Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(target fairmq-ex-custom-controller)
add_executable(${target} main.cxx)
target_link_libraries(${target} PRIVATE FairMQ)
set_target_properties(${target} PROPERTIES
  ENABLE_EXPORTS ON
)

set(test Example.custom-controller)
add_test(NAME ${test} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${target})
set_tests_properties(${test} PROPERTIES TIMEOUT 30)

