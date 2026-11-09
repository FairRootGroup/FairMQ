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
  ChannelsAccessCheck.cxx
  ChannelsAccessCheck.h
  DeprecatedFunctionsCheck.cxx
  DeprecatedFunctionsCheck.h
  DeprecatedHeadersCheck.cxx
  DeprecatedHeadersCheck.h
  Module.cxx
  NonNamespacedTypesCheck.cxx
  NonNamespacedTypesCheck.h
  PartsApiCheck.cxx
  PartsApiCheck.h
  ProgOptionsAccessorsCheck.cxx
  ProgOptionsAccessorsCheck.h
  RunDeviceCheck.cxx
  RunDeviceCheck.h
  Transfer.cxx
  Transfer.h
  UncheckedTransferResultCheck.cxx
  UncheckedTransferResultCheck.h
  UseAfterSendCheck.cxx
  UseAfterSendCheck.h
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

# libfairmq-tidy-checks: the fairmq-* checks as a plugin for a clang-tidy of
# the same LLVM release, e.g. `clang-tidy --load=libfairmq-tidy-checks.so`.
# It does not link the LLVM libraries: the clang-tidy executable has them.
set(target fairmq-tidy-plugin)
add_library(${target} MODULE $<TARGET_OBJECTS:fairmq-tidy-module>)
add_library("${PROJECT_NAME}::${target}" ALIAS ${target})
set_target_properties(${target} PROPERTIES OUTPUT_NAME fairmq-tidy-checks)
if(APPLE)
  target_link_options(${target} PRIVATE -undefined dynamic_lookup)
endif()
install(TARGETS ${target}
  EXPORT ${PROJECT_EXPORT_SET}
  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
)

# fairmq-clangd: the clangd language server plus the fairmq-* checks.
option(BUILD_TIDY_CLANGD "Build fairmq-clangd, the clangd language server with the fairmq-* checks." OFF)
if(BUILD_TIDY_CLANGD)
  if(NOT TARGET clangdMain)
    message(FATAL_ERROR "BUILD_TIDY_CLANGD=ON needs the clangd libraries "
      "(clangdMain, clangDaemon) of the found Clang ${LLVM_PACKAGE_VERSION} "
      "installation at ${Clang_DIR}.")
  endif()
  set(target fairmq-clangd)
  add_executable(${target} runClangd.cxx)
  add_executable("${PROJECT_NAME}::${target}" ALIAS ${target})
  target_compile_definitions(${target} PRIVATE
    FAIRMQ_TIDY_CLANG_RESOURCE_DIR="${FAIRMQ_TIDY_CLANG_RESOURCE_DIR}"
  )
  target_link_libraries(${target} PRIVATE fairmq-tidy-module clangdMain)
  install(TARGETS ${target}
    EXPORT ${PROJECT_EXPORT_SET}
    RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
  )
endif()
