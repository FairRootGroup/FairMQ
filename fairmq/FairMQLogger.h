/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQLogger.h
 *
 * @since 2012-12-04
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQLOGGER_H_
#define FAIRMQLOGGER_H_

#include "logger/logger.h"


// FairMQLogger helper macros 
/*
    Definition : 
    
    #define LOG(severity) BOOST_LOG_SEV(global_logger::get(),fairmq::severity)
    #define SET_LOG_CONSOLE_LEVEL(loglevel) DefaultConsoleSetFilter(fairmq::loglevel)
    #define ADD_LOG_FILESINK(filename,loglevel) DefaultAddFileSink(filename, fairmq::loglevel)
    
    enum severity_level
    {
        TRACE,
        DEBUG,
        RESULTS,
        INFO,
        WARN,
        ERROR,
        STATE,
        NOLOG
    };

    Use : 
    
    LOG(DEBUG)<<"Hello World";
    SET_LOG_CONSOLE_LEVEL(INFO); // => Print severity >= INFO to console
    ADD_LOG_FILESINK(filename,ERROR); // => Print severity >= ERROR to file (extension is added)
*/

typedef unsigned long long timestamp_t;

timestamp_t get_timestamp();

#endif /* FAIRMQLOGGER_H_ */
