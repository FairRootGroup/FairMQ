/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/* 
 * File:   logger_def.h
 * Author: winckler
 *
 * Created on September 2, 2015, 11:58 AM
 */

#ifndef LOGGER_DEF_H
#define	LOGGER_DEF_H

#include <array>
#include <fstream>
#include <sstream>

#include <boost/log/utility/manipulators/to_log.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace fairmq
{
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


    static const std::array<std::string, 8> g_LogSeverityLevelString
    {
        { 
            "TRACE",
            "DEBUG", 
            "RESULTS",
            "INFO", 
            "WARN", 
            "ERROR", 
            "STATE", 
            "NOLOG" 
        }
    };



    namespace color 
    {
        enum code 
        {
            FG_BLACK    = 30,
            FG_RED      = 31,
            FG_GREEN    = 32,
            FG_YELLOW   = 33,
            FG_BLUE     = 34,
            FG_MAGENTA  = 35,
            FG_CYAN     = 36,
            FG_WHITE    = 37,
            FG_DEFAULT  = 39,
            BG_RED      = 41,
            BG_GREEN    = 42,
            BG_BLUE     = 44,
            BG_DEFAULT  = 49
        };
    }
}

// helper function to format in color console output
template <fairmq::color::code color> 
inline std::string write_in(const std::string& text_in_bold)
{
    std::ostringstream os;
    os << "\033[01;" << color << "m" << text_in_bold << "\033[0m";

    return os.str();
}

// typedef
typedef fairmq::severity_level custom_severity_level;
#define SEVERITY_THRESHOLD custom_severity_level::TRACE
#define SEVERITY_ERROR custom_severity_level::ERROR

// tags used for log console or file formatting
struct tag_console;
struct tag_file;


// overload operator for console output
inline boost::log::formatting_ostream& operator<<
        (
            boost::log::formatting_ostream& strm,
            boost::log::to_log_manip< custom_severity_level, tag_console > const& manip
        )
{
    custom_severity_level level = manip.get();
    std::size_t idx=static_cast< std::size_t >(level);
    if ( idx < fairmq::g_LogSeverityLevelString.size() )
    {
        //strm <<" idx = "<<idx <<" ";
        switch (level)
        {
            case custom_severity_level::TRACE :
                strm << write_in<fairmq::color::FG_BLUE>(fairmq::g_LogSeverityLevelString.at(idx));
                break;
            
            case custom_severity_level::DEBUG :
                strm << write_in<fairmq::color::FG_BLUE>(fairmq::g_LogSeverityLevelString.at(idx));
                break;

            case custom_severity_level::RESULTS :
                strm << write_in<fairmq::color::FG_MAGENTA>(fairmq::g_LogSeverityLevelString.at(idx));
                break;

            case custom_severity_level::INFO :
                strm << write_in<fairmq::color::FG_GREEN>(fairmq::g_LogSeverityLevelString.at(idx));
                break;    
                
            case custom_severity_level::WARN :
                strm << write_in<fairmq::color::FG_YELLOW>(fairmq::g_LogSeverityLevelString.at(idx));
                break;

            case custom_severity_level::STATE :
                strm << write_in<fairmq::color::FG_MAGENTA>(fairmq::g_LogSeverityLevelString.at(idx));
                break;    
                
            case custom_severity_level::ERROR :
                strm << write_in<fairmq::color::FG_RED>(fairmq::g_LogSeverityLevelString.at(idx));
                break;    
                
            case custom_severity_level::NOLOG :
                strm << write_in<fairmq::color::FG_DEFAULT>(fairmq::g_LogSeverityLevelString.at(idx));
                break;
            
            default:
                break;
        }
        
    }
    else
    {
        strm    << write_in<fairmq::color::FG_RED>("Unknown log level ")
                << "(int level = "<<static_cast< int >(level)
                <<")";
    }
    return strm;
}

// overload operator for file output
inline boost::log::formatting_ostream& operator<<
        (
            boost::log::formatting_ostream& strm,
            boost::log::to_log_manip< custom_severity_level, tag_file > const& manip
        )
{
    custom_severity_level level = manip.get();
    std::size_t idx=static_cast< std::size_t >(level);
    if ( idx < fairmq::g_LogSeverityLevelString.size() )
        strm << fairmq::g_LogSeverityLevelString.at(idx);
    else
    {
        strm    << write_in<fairmq::color::FG_RED>("Unknown log level ")
                << "(int level = "<<static_cast< int >(level)
                <<")";
    }
    return strm;
}







#endif	/* LOGGER_DEF_H */

