cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
cmake_policy(VERSION 3.11...3.15)

project(sdk_tester VERSION 1 LANGUAGES CXX)

find_package(FairMQ 1.4.8 COMPONENTS sdk REQUIRED QUIET)
foreach(dep IN LISTS FairMQ_PACKAGE_DEPENDENCIES)
  if(FairMQ_${dep}_COMPONENTS)
    find_package(${dep} ${FairMQ_${dep}_VERSION} COMPONENTS ${FairMQ_${dep}_COMPONENTS} QUIET)
  else()
    find_package(${dep} ${FairMQ_${dep}_VERSION} QUIET)
  endif()
endforeach()

configure_file(main.cpp ${CMAKE_BINARY_DIR}/main.cpp @ONLY)
add_executable(tester ${CMAKE_BINARY_DIR}/main.cpp)
set_property(TARGET tester PROPERTY CXX_STANDARD 17)
target_link_libraries(tester PRIVATE FairMQ::SDK FairLogger::FairLogger)
