################################################################################
#    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

# Runs one fairmq-tidy check on one test input and verifies the result with
# FileCheck, like check_clang_tidy.py of the LLVM project:
#
#   // CHECK-MESSAGES: :[[@LINE+1]]:3: warning: <message> [<check>]
#   <code that triggers the check>
#   // CHECK-FIXES: <code after the fix>
#
# Any warning or error without a CHECK-MESSAGES line fails the test.
#
# Usage: cmake -DTIDY=<exe> -DFILECHECK=<exe> -DCHECK=<name> -DINPUT=<file>
#              -DFLAGS=<file with one compiler flag per line> -DWORKDIR=<dir>
#              [-DLOAD=<clang-tidy plugin>] -P RunCheck.cmake

foreach(var IN ITEMS TIDY FILECHECK CHECK INPUT FLAGS WORKDIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is not set")
  endif()
endforeach()

file(MAKE_DIRECTORY "${WORKDIR}")
get_filename_component(name "${INPUT}" NAME)
set(fixed "${WORKDIR}/${name}")
set(messages "${WORKDIR}/${name}.messages")

# Remove the CHECK lines from the copy, but keep the line numbers.
file(READ "${INPUT}" content)
string(REGEX REPLACE "// *CHECK-[A-Z-]+:[^\n]*" "//" content "${content}")
file(WRITE "${fixed}" "${content}")

file(STRINGS "${FLAGS}" flags)
set(load)
if(LOAD)
  set(load "--load=${LOAD}")
endif()
execute_process(
  COMMAND "${TIDY}" ${load} "--checks=-*,${CHECK}" --fix "${fixed}" -- ${flags}
  OUTPUT_FILE "${messages}"
  ERROR_VARIABLE stderr
  RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
  file(READ "${messages}" out)
  message(FATAL_ERROR "fairmq-tidy failed (${result}):\n${out}\n${stderr}")
endif()

execute_process(
  COMMAND "${FILECHECK}" "${INPUT}" --check-prefix=CHECK-MESSAGES
          "--input-file=${messages}" "--implicit-check-not={{warning|error}}:"
  RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "The diagnostics do not match the CHECK-MESSAGES lines.")
endif()

file(STRINGS "${INPUT}" fixes REGEX "// *CHECK-FIXES:")
if(fixes)
  execute_process(
    COMMAND "${FILECHECK}" "${INPUT}" --check-prefix=CHECK-FIXES
            "--input-file=${fixed}" --match-full-lines
    RESULT_VARIABLE result
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "The fixed file does not match the CHECK-FIXES lines.")
  endif()
endif()
