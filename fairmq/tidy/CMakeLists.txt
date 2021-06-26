################################################################################
#    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

set(target fairmq-tidy)
add_executable(${target}
  ModernizeNonNamespacedTypes.h
  Tool.h
  runTool.cpp
)
add_executable("${PROJECT_NAME}::${target}" ALIAS ${target})
target_compile_features(${target} PRIVATE cxx_std_17)
target_link_libraries(${target} PRIVATE clang-cpp LLVM)
target_include_directories(${target} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
)
install(TARGETS ${target}
  EXPORT ${PROJECT_EXPORT_SET}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)
