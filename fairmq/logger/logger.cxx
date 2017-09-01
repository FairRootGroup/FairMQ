/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "logger.h"
#include <boost/version.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#if BOOST_VERSION < 105600
#include "fairroot_null_deleter.h"
using empty_deleter_t = fairroot::null_deleter;
#else
#include <boost/core/null_deleter.hpp>
using empty_deleter_t = boost::null_deleter;
#endif

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <fstream>
#include <ostream>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;

BOOST_LOG_GLOBAL_LOGGER_INIT(global_logger, src::severity_logger_mt)
{
    src::severity_logger_mt<custom_severity_level> global_logger;
    global_logger.add_attribute("TimeStamp", attrs::local_clock());
    DefaultConsoleInit();
    return global_logger;
}

namespace FairMQ
{
namespace Logger
{
    std::vector<boost::shared_ptr<boost::log::sinks::basic_sink_frontend>> sinkList;// global var
} // end Logger namespace
} // end FairMQ namespace

void RemoveRegisteredSinks()
{
    if (FairMQ::Logger::sinkList.size() > 0)
    {
        for (const auto& sink : FairMQ::Logger::sinkList)
        {
            logging::core::get()->remove_sink(sink);
        }
        FairMQ::Logger::sinkList.clear();
    }
}

void reinit_logger(bool color, const std::string& filename, custom_severity_level threshold)
{
    BOOST_LOG_SEV(global_logger::get(), custom_severity_level::NOLOG) << "";
    RemoveRegisteredSinks();
    DefaultConsoleInit(color);
    if (threshold != SEVERITY_NOLOG)
    {
        if (!filename.empty())
        {
            DefaultAddFileSink(filename, threshold);
        }
    }
}

// console sink related functions
void DefaultConsoleInit(bool color/* = true*/)
{
    // add a text sink
    using text_sink = sinks::synchronous_sink<sinks::text_ostream_backend>;

    RemoveRegisteredSinks();

    // CONSOLE - all severity except error
    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
    // add "console" output stream to our sink
    sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::clog, empty_deleter_t()));

    // specify the format of the log message 
    if (color)
    {
        sink->set_formatter(&InitLogFormatter<tag_console>);
    }
    else
    {
        sink->set_formatter(&InitLogFormatter<tag_file>);
    }

    sink->set_filter(severity != SEVERITY_ERROR && severity < SEVERITY_NOLOG);
    // add sink to the core
    FairMQ::Logger::sinkList.push_back(sink);
    logging::core::get()->add_sink(sink);

    // CONSOLE - only severity error
    boost::shared_ptr<text_sink> sink_error = boost::make_shared<text_sink>();
    sink_error->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cerr, empty_deleter_t()));

    if (color)
    {
        sink_error->set_formatter(&InitLogFormatter<tag_console>);
    }
    else
    {
        sink_error->set_formatter(&InitLogFormatter<tag_file>);
    }

    sink_error->set_filter(severity == SEVERITY_ERROR);
    FairMQ::Logger::sinkList.push_back(sink_error);
    logging::core::get()->add_sink(sink_error);
}

int DefaultConsoleSetFilter(custom_severity_level threshold)
{
    if (FairMQ::Logger::sinkList.size()>=2)
    {
        FairMQ::Logger::sinkList.at(0)->set_filter([threshold](const boost::log::attribute_value_set& attr_set)
        {
            auto sev = attr_set["Severity"].extract<custom_severity_level>();
            auto mainConsoleSinkCondition = (sev != SEVERITY_ERROR) && (sev < SEVERITY_NOLOG);
            return mainConsoleSinkCondition && (sev>=threshold);
        });

        FairMQ::Logger::sinkList.at(1)->set_filter([threshold](const boost::log::attribute_value_set& attr_set)
        {
            auto sev = attr_set["Severity"].extract<custom_severity_level>();
            auto errorConsoleSinkCondition = sev == SEVERITY_ERROR;
            return errorConsoleSinkCondition && (sev>=threshold);
        });
        return 0;
    }
    else
    {
        return 1;
    }

    return 0;
}

// file sink related functions
void DefaultAddFileSink(const std::string& filename, custom_severity_level threshold)
{
    // add a text sink
    std::string formatted_filename(filename);
    formatted_filename += "_%Y-%m-%d_%H-%M-%S.%N.log";
    AddFileSink([threshold](const boost::log::attribute_value_set& attr_set)
        {
            auto sev = attr_set["Severity"].extract<custom_severity_level>();
            return (sev >= threshold) && (sev < SEVERITY_NOLOG);
        },
        boost::log::keywords::file_name = formatted_filename, 
        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
        // rotate at midnight every day
        boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
        // log collector,
        // -- maximum total size of the stored log files is 1GB.
        // -- minimum free space on the drive is 2GB
        boost::log::keywords::max_size = 1000 * 1024 * 1024,
        boost::log::keywords::min_free_space = 2000 * 1024 * 1024,
        boost::log::keywords::auto_flush = true
        //keywords::time_based_rotation = &is_it_time_to_rotate 
        );
}
