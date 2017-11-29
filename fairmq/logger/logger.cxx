/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "logger.h"

#include <boost/version.hpp>

#include <boost/log/core/core.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/log/support/date_time.hpp>

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/to_log.hpp>
#include <boost/log/utility/formatting_ostream.hpp>

#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/basic_sink_frontend.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/keyword.hpp>

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
#include <string>
#include <vector>
#include <sstream>

using namespace std;
namespace blog = boost::log;
namespace bptime = boost::posix_time;

struct TagConsole;
struct TagFile;

BOOST_LOG_ATTRIBUTE_KEYWORD(fairmq_logger_timestamp, "TimeStamp", bptime::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", fair::mq::logger::SeverityLevel)

BOOST_LOG_GLOBAL_LOGGER_INIT(global_logger, blog::sources::severity_logger_mt)
{
    blog::sources::severity_logger_mt<fair::mq::logger::SeverityLevel> globalLogger;
    globalLogger.add_attribute("TimeStamp", blog::attributes::local_clock());
    fair::mq::logger::DefaultConsoleInit();
    return globalLogger;
}

namespace fair
{
namespace mq
{
namespace logger
{

vector<boost::shared_ptr<blog::sinks::basic_sink_frontend>> sinkList;// global var

void InitConsoleLogFormatter(const blog::record_view& view, blog::formatting_ostream& os)
{
    os << "[\033[01;36m";

    auto dateTimeFormatter = blog::expressions::stream << blog::expressions::format_date_time<bptime::ptime>("TimeStamp", "%H:%M:%S");
    dateTimeFormatter(view, os);

    os << "\033[0m][" << view.attribute_values()["Severity"].extract<SeverityLevel, TagConsole>() << "] " << view.attribute_values()["Message"].extract<string>();
}

void InitFileLogFormatter(const blog::record_view& view, blog::formatting_ostream& os)
{
    os << "[";

    auto dateTimeFormatter = blog::expressions::stream << blog::expressions::format_date_time<bptime::ptime>("TimeStamp", "%H:%M:%S");
    dateTimeFormatter(view, os);

    os << "][" << view.attribute_values()["Severity"].extract<SeverityLevel, TagFile>() << "] " << view.attribute_values()["Message"].extract<string>();
}


// helper function to format in color console output
string writeIn(const string& textInBold, color::Code color)
{
    ostringstream os;
    os << "\033[01;" << color << "m" << textInBold << "\033[0m";
    return os.str();
}

// overload operator for console output
blog::formatting_ostream& operator<<(blog::formatting_ostream& stream, blog::to_log_manip<SeverityLevel, TagConsole> const& manip)
{
    SeverityLevel level = manip.get();
    size_t idx = static_cast<size_t>(level);
    if (idx < gLogSeverityLevelString.size())
    {
        switch (level)
        {
            case SeverityLevel::TRACE:
                stream << writeIn(gLogSeverityLevelString.at(idx), color::FG_BLUE);
                break;

            case SeverityLevel::DEBUG:
                stream << writeIn(gLogSeverityLevelString.at(idx), color::FG_BLUE);
                break;

            case SeverityLevel::RESULTS:
                stream << writeIn(gLogSeverityLevelString.at(idx), color::FG_MAGENTA);
                break;

            case SeverityLevel::INFO:
                stream << writeIn(gLogSeverityLevelString.at(idx), color::FG_GREEN);
                break;

            case SeverityLevel::WARN:
                stream << writeIn(gLogSeverityLevelString.at(idx), color::FG_YELLOW);
                break;

            case SeverityLevel::STATE:
                stream << writeIn(gLogSeverityLevelString.at(idx), color::FG_MAGENTA);
                break;

            case SeverityLevel::ERROR:
                stream << writeIn(gLogSeverityLevelString.at(idx), color::FG_RED);
                break;

            case SeverityLevel::NOLOG:
                stream << writeIn(gLogSeverityLevelString.at(idx), color::FG_DEFAULT);
                break;

            default:
                break;
        }
    }
    else
    {
        stream << writeIn("Unknown log level ", color::FG_RED) << "(int level = " << static_cast<int>(level) << ")";
    }
    return stream;
}

// overload operator for file output
blog::formatting_ostream& operator<<(blog::formatting_ostream& stream, blog::to_log_manip<SeverityLevel, TagFile> const& manip)
{
    SeverityLevel level = manip.get();
    size_t idx = static_cast<size_t>(level);
    if (idx < gLogSeverityLevelString.size())
    {
        stream << gLogSeverityLevelString.at(idx);
    }
    else
    {
        stream << writeIn("Unknown log level ", color::FG_RED) << "(int level = " << static_cast<int>(level) << ")";
    }
    return stream;
}

void RemoveRegisteredSinks()
{
    if (sinkList.size() > 0)
    {
        for (const auto& sink : sinkList)
        {
            blog::core::get()->remove_sink(sink);
        }
        sinkList.clear();
    }
}

void ReinitLogger(bool color, const string& filename, SeverityLevel level)
{
    BOOST_LOG_SEV(global_logger::get(), SeverityLevel::NOLOG) << "";
    RemoveRegisteredSinks();
    DefaultConsoleInit(color);
    if (level != SeverityLevel::NOLOG)
    {
        if (!filename.empty())
        {
            DefaultAddFileSink(filename, level);
        }
    }
}

// console sink related functions
void DefaultConsoleInit(bool color/* = true*/)
{
    // add a text sink
    using TextSink = blog::sinks::synchronous_sink<blog::sinks::text_ostream_backend>;

    RemoveRegisteredSinks();

    // CONSOLE - all severity except error
    boost::shared_ptr<TextSink> sink = boost::make_shared<TextSink>();
    // add "console" output stream to our sink
    sink->locked_backend()->add_stream(boost::shared_ptr<ostream>(&clog, empty_deleter_t()));

    // specify the format of the log message
    if (color)
    {
        sink->set_formatter(&InitConsoleLogFormatter);
    }
    else
    {
        sink->set_formatter(&InitFileLogFormatter);
    }

    sink->set_filter(severity != SeverityLevel::ERROR && severity < SeverityLevel::NOLOG);
    // add sink to the core
    sinkList.push_back(sink);
    blog::core::get()->add_sink(sink);

    // CONSOLE - only severity error
    boost::shared_ptr<TextSink> sinkError = boost::make_shared<TextSink>();
    sinkError->locked_backend()->add_stream(boost::shared_ptr<ostream>(&cerr, empty_deleter_t()));

    if (color)
    {
        sinkError->set_formatter(&InitConsoleLogFormatter);
    }
    else
    {
        sinkError->set_formatter(&InitFileLogFormatter);
    }

    sinkError->set_filter(severity == SeverityLevel::ERROR);
    sinkList.push_back(sinkError);
    blog::core::get()->add_sink(sinkError);
}

int DefaultConsoleSetFilter(SeverityLevel level)
{
    if (sinkList.size() >= 2)
    {
        sinkList.at(0)->set_filter([level](const blog::attribute_value_set& attrSet)
        {
            auto sev = attrSet["Severity"].extract<SeverityLevel>();
            auto mainConsoleSinkCondition = (sev != SeverityLevel::ERROR) && (sev < SeverityLevel::NOLOG);
            return mainConsoleSinkCondition && (sev >= level);
        });

        sinkList.at(1)->set_filter([level](const blog::attribute_value_set& attrSet)
        {
            auto sev = attrSet["Severity"].extract<SeverityLevel>();
            auto errorConsoleSinkCondition = sev == SeverityLevel::ERROR;
            return errorConsoleSinkCondition && (sev >= level);
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
void DefaultAddFileSink(const string& filename, SeverityLevel level)
{
    // add a text sink
    string formattedFilename(filename);
    formattedFilename += "_%Y-%m-%d_%H-%M-%S.%N.log";

    // add a text sink
    using SinkBackend = blog::sinks::text_file_backend;
    using Sink = blog::sinks::synchronous_sink<SinkBackend>;

    boost::shared_ptr<SinkBackend> backend = boost::make_shared<SinkBackend>(
        blog::keywords::file_name = formattedFilename,
        blog::keywords::rotation_size = 10 * 1024 * 1024,
        // rotate at midnight every day
        blog::keywords::time_based_rotation = blog::sinks::file::rotation_at_time_point(0, 0, 0),
        // log collector,
        // -- maximum total size of the stored log files is 1GB.
        // -- minimum free space on the drive is 2GB
        blog::keywords::max_size = 1000 * 1024 * 1024,
        blog::keywords::min_free_space = 2000 * 1024 * 1024,
        blog::keywords::auto_flush = true
        //keywords::time_based_rotation = &is_it_time_to_rotate
    );
    boost::shared_ptr<Sink> sink = boost::make_shared<Sink>(backend);

    // specify the format of the log message
    sink->set_formatter(&InitFileLogFormatter);

    // forward lambda for setting the filter
    sink->set_filter([level](const blog::attribute_value_set& attrSet)
    {
        auto sev = attrSet["Severity"].extract<SeverityLevel>();
        return (sev >= level) && (sev < SeverityLevel::NOLOG);
    });

    // add file sinks to core and list
    blog::core::get()->add_sink(sink);
    sinkList.push_back(sink);
}

} // namespace logger
} // namespace mq
} // namespace fair
