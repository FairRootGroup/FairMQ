################################################################################
# Copyright (C) 2021-2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

if(NOT TARGET clangTidy OR NOT TARGET clangTidyMain)
  message(FATAL_ERROR "BUILD_TIDY_TOOL=ON needs the clang-tidy libraries "
    "(clangTidy, clangTidyMain) of the found Clang ${LLVM_PACKAGE_VERSION} "
    "installation at ${Clang_DIR}. Install the clang-tidy development package "
    "(e.g. libclang-<version>-dev on Debian/Ubuntu).")
endif()

# The clang driver searches its builtin headers relative to the executable.
# Remember the resource directory of the LLVM installation that we build
# against, so that fairmq-tidy works from any install location.
foreach(candidate IN ITEMS "${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION_MAJOR}"
                           "${LLVM_LIBRARY_DIR}/clang/${LLVM_PACKAGE_VERSION}")
  if(EXISTS "${candidate}/include/stddef.h")
    set(FAIRMQ_TIDY_CLANG_RESOURCE_DIR "${candidate}"
      CACHE PATH "Clang resource directory used by fairmq-tidy")
    break()
  endif()
endforeach()
if(NOT FAIRMQ_TIDY_CLANG_RESOURCE_DIR)
  message(FATAL_ERROR "Cannot find the Clang resource directory below "
    "${LLVM_LIBRARY_DIR}/clang. Set FAIRMQ_TIDY_CLANG_RESOURCE_DIR to the "
    "output of `clang -print-resource-dir`.")
endif()

# The fairmq-* checks as a clang-tidy module.
add_library(fairmq-tidy-module OBJECT
  Module.cxx
  NonNamespacedTypesCheck.cxx
  NonNamespacedTypesCheck.h
)
target_compile_features(fairmq-tidy-module PUBLIC cxx_std_17)
target_include_directories(fairmq-tidy-module
  SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS}
)
target_include_directories(fairmq-tidy-module PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(fairmq-tidy-module PUBLIC -fno-rtti)
endif()
set_target_properties(fairmq-tidy-module PROPERTIES POSITION_INDEPENDENT_CODE ON)

# fairmq-tidy: the clang-tidy driver plus the fairmq-* checks.
set(target fairmq-tidy)
add_executable(${target} runTidy.cxx)
add_executable("${PROJECT_NAME}::${target}" ALIAS ${target})
target_compile_definitions(${target} PRIVATE
  FAIRMQ_TIDY_CLANG_RESOURCE_DIR="${FAIRMQ_TIDY_CLANG_RESOURCE_DIR}"
)
target_link_libraries(${target} PRIVATE fairmq-tidy-module clangTidyMain)
install(TARGETS ${target}
  EXPORT ${PROJECT_EXPORT_SET}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)
