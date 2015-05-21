 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             # 
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #  
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################
# Create a library called "ProgOptionTest" 

# Test fair-prog-opt

  # scripts
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/testMQOptions1.sh.in ${CMAKE_BINARY_DIR}/bin/testMQOptions1.sh )
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/testMQOptions2.sh.in ${CMAKE_BINARY_DIR}/bin/testMQOptions2.sh )
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/testMQOptions3.sh.in ${CMAKE_BINARY_DIR}/bin/testMQOptions3.sh )
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/testMQOptions4.sh.in ${CMAKE_BINARY_DIR}/bin/testMQOptions4.sh )
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/testMQOptions5.sh.in ${CMAKE_BINARY_DIR}/bin/testMQOptions5.sh )
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/start-bsampler-sink.sh.in ${CMAKE_BINARY_DIR}/bin/start-bsampler-sink.sh )
  EXEC_PROGRAM(/bin/chmod ARGS "u+x  ${CMAKE_BINARY_DIR}/bin/testMQOptions1.sh")
  EXEC_PROGRAM(/bin/chmod ARGS "u+x  ${CMAKE_BINARY_DIR}/bin/testMQOptions2.sh")
  EXEC_PROGRAM(/bin/chmod ARGS "u+x  ${CMAKE_BINARY_DIR}/bin/testMQOptions3.sh")
  EXEC_PROGRAM(/bin/chmod ARGS "u+x  ${CMAKE_BINARY_DIR}/bin/testMQOptions4.sh")
  EXEC_PROGRAM(/bin/chmod ARGS "u+x  ${CMAKE_BINARY_DIR}/bin/testMQOptions5.sh")
  EXEC_PROGRAM(/bin/chmod ARGS "u+x  ${CMAKE_BINARY_DIR}/bin/start-bsampler-sink.sh")

  # Config/xml/json example file
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/ConfigFileTest.cfg.in ${CMAKE_BINARY_DIR}/bin/ConfigFileTest.cfg)
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/testXML.xml ${CMAKE_BINARY_DIR}/bin/testXML.xml )
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/testJSON.json ${CMAKE_BINARY_DIR}/bin/testJSON.json )
  configure_file( ${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/macro/bsampler-sink.json ${CMAKE_BINARY_DIR}/bin/bsampler-sink.json )
  


set(INCLUDE_DIRECTORIES
${CMAKE_SOURCE_DIR}/fairmq
${CMAKE_SOURCE_DIR}/fairmq/options
${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/lib
${CMAKE_SOURCE_DIR}/fairmq/options/ProgOptionTest/run
)

Set(SYSTEM_INCLUDE_DIRECTORIES
  ${SYSTEM_INCLUDE_DIRECTORIES}
)

include_directories(${INCLUDE_DIRECTORIES})
include_directories(${SYSTEM_INCLUDE_DIRECTORIES})

set(LINK_DIRECTORIES
${Boost_LIBRARY_DIRS}
) 

link_directories(${LINK_DIRECTORIES})

Set(SRCS 
    lib/FairMQParser.cxx
)

Set(LIBRARY_NAME ProgOptionTest)

If (Boost_FOUND)
  Set(DEPENDENCIES
    boost_system boost_program_options FairMQ
    )
EndIf (Boost_FOUND)

GENERATE_LIBRARY()


If (Boost_FOUND)
  Set(Exe_Names
    runtestMQOption1
    runtestMQOption2
    runOptTestSampler
    runOptTestSink
  )


  set(Exe_Source    
    run/testMQoptions1.cxx
    run/testMQoptions2.cxx
    run/runOptTestSampler.cxx
    run/runOptTestSink.cxx
    )

  List(LENGTH Exe_Names _length)
  Math(EXPR _length ${_length}-1)

  ForEach(_file RANGE 0 ${_length})
    List(GET Exe_Names ${_file} _name)
    List(GET Exe_Source ${_file} _src)
    Set(EXE_NAME ${_name})
    Set(SRCS ${_src})
    Set(DEPENDENCIES ProgOptionTest)
    GENERATE_EXECUTABLE()
  EndForEach(_file RANGE 0 ${_length})
EndIf (Boost_FOUND)
