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
#ifndef FAIR_MQ_LOGGER_H
#define FAIR_MQ_LOGGER_H

#define BOOST_LOG_DYN_LINK 1 // necessary when linking the boost_log library dynamically
#define FUSION_MAX_VECTOR_SIZE 20

#ifdef DEBUG
#undef DEBUG
#warning "The symbol 'DEBUG' is used in FairMQLogger. undefining..."
#endif

#include <boost/version.hpp>

#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <array>

namespace fair
{
namespace mq
{
namespace logger
{

enum SeverityLevel
{
    TRACE,
    DEBUG,
    RESULTS,
    INFO,
    STATE,
    WARN,
    ERROR,
    NOLOG
};

static const std::array<std::string, 8> gLogSeverityLevelString
{
    {
        "TRACE",
        "DEBUG",
        "RESULTS",
        "INFO",
        "STATE",
        "WARN",
        "ERROR",
        "NOLOG"
    }
};

namespace color
{

enum Code
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

} // namespace color

void ReinitLogger(bool color, const std::string& filename = "", SeverityLevel level = SeverityLevel::NOLOG);
void RemoveRegisteredSinks();

// console sink related functions
void DefaultConsoleInit(bool color = true);
int DefaultConsoleSetFilter(SeverityLevel level);

// file sink related functions
void DefaultAddFileSink(const std::string& filename, SeverityLevel level);

} // namespace logger
} // namespace mq
} // namespace fair

// register a global logger (declaration)
BOOST_LOG_GLOBAL_LOGGER(global_logger, boost::log::sources::severity_logger_mt<fair::mq::logger::SeverityLevel>)

// helper macros

// global macros (core). Level filters are set globally here, that is to all register sinks
// add empty string if boost 1.59.0 (see : https://svn.boost.org/trac/boost/ticket/11549 )
#if BOOST_VERSION == 105900
#define LOG(severity) BOOST_LOG_SEV(global_logger::get(), fair::mq::logger::SeverityLevel::severity) << ""
#define MQLOG(severity) BOOST_LOG_SEV(global_logger::get(), fair::mq::logger::SeverityLevel::severity) << ""
#else
#define LOG(severity) BOOST_LOG_SEV(global_logger::get(), fair::mq::logger::SeverityLevel::severity)
#define MQLOG(severity) BOOST_LOG_SEV(global_logger::get(), fair::mq::logger::SeverityLevel::severity)
#endif

#define SET_LOG_CONSOLE_LEVEL(loglevel) DefaultConsoleSetFilter(fair::mq::logger::SeverityLevel::loglevel)
#define ADD_LOG_FILESINK(filename, loglevel) DefaultAddFileSink(filename, fair::mq::logger::SeverityLevel::loglevel)

#endif // FAIR_MQ_LOGGER_H
