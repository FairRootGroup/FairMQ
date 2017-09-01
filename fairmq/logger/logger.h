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
#define LOGGER_H

#define BOOST_LOG_DYN_LINK 1 // necessary when linking the boost_log library dynamically
#define FUSION_MAX_VECTOR_SIZE 20

#ifdef DEBUG
#undef DEBUG
#warning "The symbol 'DEBUG' is used in FairMQLogger. undefining..."
#endif

// std
#include <type_traits>
#include <cstddef>
#include <iostream>

// boost
#include <boost/version.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>

#include <boost/log/sinks/basic_sink_frontend.hpp>
// fairmq
#include "logger_def.h"

// Note : the following types and values must be defined in the included logger_def.h :
// 1- custom_severity_level 
// 2- SEVERITY_THRESHOLD
// 3- tag_console
// 4- tag_file


#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif

// register a global logger (declaration)
BOOST_LOG_GLOBAL_LOGGER(global_logger, boost::log::sources::severity_logger_mt<custom_severity_level>)

BOOST_LOG_ATTRIBUTE_KEYWORD(fairmq_logger_timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", custom_severity_level)

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif


namespace FairMQ
{
namespace Logger
{
    // common
    extern std::vector<boost::shared_ptr< boost::log::sinks::basic_sink_frontend > >sinkList;// global var
}}
    void reinit_logger(bool color, const std::string& filename = "", custom_severity_level threshold = SEVERITY_NOLOG);
    void RemoveRegisteredSinks();

    template<typename T>
    void InitLogFormatter(const boost::log::record_view &view, boost::log::formatting_ostream &os)
    {
        os << "[";

        if (std::is_same<T,tag_console>::value)
        {
            os << "\033[01;36m";
        }

        auto date_time_formatter = boost::log::expressions::stream << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S");
        date_time_formatter(view, os);

        if (std::is_same<T,tag_console>::value)
        {
            os << "\033[0m";
        }

        os << "]"
           << "["
           << view.attribute_values()["Severity"].extract<custom_severity_level, T>()
           << "] "
           //<< " - " 
           << view.attribute_values()["Message"].extract<std::string>();
    }


    template<typename FunT>
    int SetSinkFilterImpl(std::size_t index, FunT&& func)
    {
        if(index<FairMQ::Logger::sinkList.size())
        {
            FairMQ::Logger::sinkList.at(index)->set_filter(std::forward<FunT>(func));
        }
        return 0;
    }

    // console sink related functions
   

        void DefaultConsoleInit(bool color = true);
        int DefaultConsoleSetFilter(custom_severity_level threshold);

    

    // file sink related functions
   
        void DefaultAddFileSink(const std::string& filename, custom_severity_level threshold);
        
        template<typename FunT, typename... Args>
        void AddFileSink(FunT&& func, Args&&... args)
        {
            // add a text sink
            using sink_backend_t = boost::log::sinks::text_file_backend;
            using sink_t = boost::log::sinks::synchronous_sink<sink_backend_t>;

            // forward keywords args for setting log file properties
            boost::shared_ptr<sink_backend_t> backend = boost::make_shared<sink_backend_t>(std::forward<Args>(args)...);
            boost::shared_ptr<sink_t> sink = boost::make_shared<sink_t>(backend);

            // specify the format of the log message 
            sink->set_formatter(&InitLogFormatter<tag_file>);

            // forward lambda for setting the filter
            sink->set_filter(std::forward<FunT>(func));
                    
            // add file sinks to core and list
            boost::log::core::get()->add_sink(sink);
            FairMQ::Logger::sinkList.push_back(sink);

        }


// helper macros 

// global macros (core). Level filters are set globally here, that is to all register sinks
// add empty string if boost 1.59.0 (see : https://svn.boost.org/trac/boost/ticket/11549 )
#if BOOST_VERSION == 105900
#define LOG(severity) BOOST_LOG_SEV(global_logger::get(),custom_severity_level::severity) << ""
#define MQLOG(severity) BOOST_LOG_SEV(global_logger::get(),custom_severity_level::severity) << ""
#else
#define LOG(severity) BOOST_LOG_SEV(global_logger::get(),custom_severity_level::severity)
#define MQLOG(severity) BOOST_LOG_SEV(global_logger::get(),custom_severity_level::severity)
#endif

#define SET_LOG_CONSOLE_LEVEL(loglevel) DefaultConsoleSetFilter(custom_severity_level::loglevel)
#define ADD_LOG_FILESINK(filename,loglevel) DefaultAddFileSink(filename, custom_severity_level::loglevel)

// Use : SET_LOG_CONSOLE_LEVEL(INFO); ADD_LOG_FILESINK(filename,ERROR);
#endif
