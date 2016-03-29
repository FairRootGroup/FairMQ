 ################################################################################
 #    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    #
 #                                                                              #
 #              This software is distributed under the terms of the             # 
 #         GNU Lesser General Public Licence version 3 (LGPL) version 3,        #  
 #                  copied verbatim in the file "LICENSE"                       #
 ################################################################################
# Create a library  

set(INCLUDE_DIRECTORIES
    ${CMAKE_SOURCE_DIR}/fairmq/logger
    ${CMAKE_SOURCE_DIR}/fairmq/logger/run
    )

include_directories(${INCLUDE_DIRECTORIES})

set(SYSTEM_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})
include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

set(LINK_DIRECTORIES ${Boost_LIBRARY_DIRS}) 

link_directories(${LINK_DIRECTORIES})

set(SRCS logger.cxx)

set(LIBRARY_NAME fairmq_logger)

set(DEPENDENCIES
    ${Boost_LOG_LIBRARY}
    ${Boost_LOG_SETUP_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    pthread
  )

install(FILES logger.h logger_def.h DESTINATION include/logger)
if("${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}" VERSION_LESS "1.56")
    install(FILES fairroot_null_deleter.h DESTINATION include/logger)
endif()



GENERATE_LIBRARY()

# generate test executable
set(EXE_NAME runtestLogger)
set(SRCS run/testLogger.cxx)
set(DEPENDENCIES fairmq_logger)
GENERATE_EXECUTABLE()

