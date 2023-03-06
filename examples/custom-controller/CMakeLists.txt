################################################################################
# Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(name "custom-controller")
set(exe "${exe_prefix}-${name}")
add_executable(${exe} main.cxx)
target_link_libraries(${exe} PRIVATE FairMQ)
set_target_properties(${exe} PROPERTIES ENABLE_EXPORTS ON)

set(test "${testsuite}.${name}")
add_test(NAME ${test} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${exe})
set_tests_properties(${test} PROPERTIES TIMEOUT 30)
if(lsan_options)
  set_tests_properties(${test} PROPERTIES ENVIRONMENT_MODIFICATION ${lsan_options})
endif()
