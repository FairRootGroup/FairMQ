################################################################################
#    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

find_program(FAIRMQ_TIDY_FILECHECK NAMES FileCheck FileCheck-${LLVM_VERSION_MAJOR}
  HINTS "${LLVM_TOOLS_BINARY_DIR}"
  DOC "FileCheck executable for the fairmq-tidy tests"
)
if(NOT FAIRMQ_TIDY_FILECHECK)
  message(WARNING "FileCheck not found (install the LLVM tools, e.g. "
    "llvm-${LLVM_VERSION_MAJOR}-tools). Skipping the fairmq-tidy tests.")
  return()
endif()

# The test inputs include the real FairMQ headers. Pass the usage requirements
# of the FairMQ target to the clang frontend, one flag per line.
# Drop the default system include directory: a second copy in the search
# path breaks the #include_next chain of the C++ standard library headers.
set(incs "$<TARGET_PROPERTY:FairMQ,INTERFACE_INCLUDE_DIRECTORIES>")
set(incs "$<REMOVE_DUPLICATES:$<FILTER:${incs},EXCLUDE,^/usr/include/?$>>")
set(defs "$<TARGET_PROPERTY:FairMQ,INTERFACE_COMPILE_DEFINITIONS>")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/flags.txt" CONTENT
"-std=c++17
$<$<BOOL:${incs}>:-isystem\n$<JOIN:${incs},\n-isystem\n>>
$<$<BOOL:${defs}>:-D$<JOIN:${defs},\n-D>>
")

file(GLOB inputs CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/fairmq-*.cxx")
foreach(input IN LISTS inputs)
  get_filename_component(check "${input}" NAME_WE)
  add_test(NAME "tidy.${check}"
    COMMAND ${CMAKE_COMMAND}
      "-DTIDY=$<TARGET_FILE:FairMQ::fairmq-tidy>"
      "-DFILECHECK=${FAIRMQ_TIDY_FILECHECK}"
      "-DCHECK=${check}"
      "-DINPUT=${input}"
      "-DFLAGS=${CMAKE_CURRENT_BINARY_DIR}/flags.txt"
      "-DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/${check}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/RunCheck.cmake"
  )
  set_tests_properties("tidy.${check}" PROPERTIES TIMEOUT 120)
endforeach()
