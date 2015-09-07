/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/* 
 * File:   logger.h
 * Author: winckler
 *
 * Created on August 21, 2015, 6:12 PM
 */
#ifndef LOGGER_H
#define	LOGGER_H

#define BOOST_LOG_DYN_LINK 1 // necessary when linking the boost_log library dynamically

// std
#include <type_traits>
#include <cstddef>
#include <iostream>

// boost
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/attr.hpp>

// fairmq
#include "logger_def.h"

// Note : the following types and values must be defined in the included logger_def.h :
// 1- custom_severity_level 
// 2- SEVERITY_THRESHOLD
// 3- tag_console
// 4- tag_file

// Note : operation enum temporary : (until we replace it with c++11 lambda expression)
namespace log_op
{
    enum operation
    {
        EQUAL,
        GREATER_THAN,
        GREATER_EQ_THAN,
        LESS_THAN,
        LESS_EQ_THAN
    };
}


// declaration of the init function for the global logger
void init_log_console();
void init_log_file( const std::string& filename, 
                    custom_severity_level threshold=SEVERITY_THRESHOLD, 
                    log_op::operation=log_op::GREATER_EQ_THAN,
                    const std::string& id=""
                  );

void init_new_file( const std::string& filename, 
                    custom_severity_level threshold, 
                    log_op::operation op
                   );

void set_global_log_level(  log_op::operation op=log_op::GREATER_EQ_THAN, 
                            custom_severity_level threshold=SEVERITY_THRESHOLD );
void set_global_log_level_operation(  log_op::operation op=log_op::GREATER_EQ_THAN, 
                            custom_severity_level threshold=SEVERITY_THRESHOLD );
// register a global logger (declaration)
BOOST_LOG_GLOBAL_LOGGER(global_logger, boost::log::sources::severity_logger_mt<custom_severity_level>)

BOOST_LOG_ATTRIBUTE_KEYWORD(fairmq_logger_timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", custom_severity_level)        


template<typename T>
void init_log_formatter(const boost::log::record_view &view, boost::log::formatting_ostream &os)
{
    os      << "[" ;
    
    if(std::is_same<T,tag_console>::value)
        os<<"\033[01;36m";
    
    auto date_time_formatter = 
        boost::log::expressions::stream 
            << boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "%H:%M:%S");
    date_time_formatter(view, os);
    
    if(std::is_same<T,tag_console>::value)
        os<<"\033[0m";
    
    os      << "]"
            << "[" 
            << view.attribute_values()["Severity"].extract<custom_severity_level,T>()
            << "] "
            //<< " - " 
            << view.attribute_values()["Message"].extract<std::string>();
}



// helper macros 

// global macros (core). Level filters are set globally here, that is to all register sinks
#define LOG(severity) BOOST_LOG_SEV(global_logger::get(),custom_severity_level::severity)
#define MQLOG(severity) BOOST_LOG_SEV(global_logger::get(),custom_severity_level::severity)
#define SET_LOG_LEVEL(loglevel) boost::log::core::get()->set_filter(severity >= custom_severity_level::loglevel);
#define SET_LOG_FILTER(op,loglevel)  set_global_log_level(log_op::op,custom_severity_level::loglevel)

// local init macros (sinks)
// Notes : when applying a filter to the sink, and then to the core, the resulting filter will 
//         be the intersection of the two sets defined by the two filters, i.e., core and sinks
// filename : path to file name without extension
#define INIT_LOG_FILE(filename) init_log_file(filename);
#define INIT_LOG_FILE_LVL(filename,loglevel) init_log_file(filename,custom_severity_level::loglevel);
#define INIT_LOG_FILE_FILTER(filename,op,loglevel) init_log_file(filename,custom_severity_level::loglevel,log_op::op);
//INIT_LOG_FILE_FILTER_MP : add id to log filename for multiprocess
#define INIT_LOG_FILE_FILTER_MP(filename,op,loglevel,id) init_log_file(filename,custom_severity_level::loglevel,log_op::GREATER_EQ_THAN,id);

// create new file without formatting
#define INIT_NEW_FILE(filename,op,loglevel) init_new_file(filename,custom_severity_level::loglevel,log_op::op);

        
#endif
