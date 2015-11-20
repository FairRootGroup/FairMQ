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
typedef fairroot::null_deleter empty_deleter_t;
#else
#include <boost/core/null_deleter.hpp>
typedef boost::null_deleter empty_deleter_t;
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
    init_log_console();
    return global_logger;
}

void init_log_console(bool color_format)
{
    // add a text sink
    typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
    logging::core::get()->remove_all_sinks();
    
    // CONSOLE - all severity except error
    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
    // add "console" output stream to our sink
    sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cout, empty_deleter_t()));
    
    // specify the format of the log message 
    if(color_format)
        sink->set_formatter(&init_log_formatter<tag_console>);
    else
        sink->set_formatter(&init_log_formatter<tag_file>);
    
    sink->set_filter(severity != SEVERITY_ERROR && severity < SEVERITY_NOLOG);
    // add sink to the core
    logging::core::get()->add_sink(sink);
    
    
    // CONSOLE - only severity error
    boost::shared_ptr<text_sink> sink_error = boost::make_shared<text_sink>();
    sink_error->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cerr, empty_deleter_t()));
    
    if(color_format)
        sink_error->set_formatter(&init_log_formatter<tag_console>);
    else
        sink_error->set_formatter(&init_log_formatter<tag_file>);
    
    sink_error->set_filter(severity == SEVERITY_ERROR);
    logging::core::get()->add_sink(sink_error);
}

void reinit_logger(bool color_format)
{
    LOG(NOLOG)<<"";
    logging::core::get()->remove_all_sinks();
    init_log_console(color_format);
}


void init_log_file(const std::string& filename, custom_severity_level threshold, log_op::operation op, const std::string& id)
{
    // add a text sink
    std::string formatted_filename(filename);
    formatted_filename+=id;
    formatted_filename+="_%Y-%m-%d_%H-%M-%S.%N.log";
    boost::shared_ptr< sinks::text_file_backend > backend =
        boost::make_shared< sinks::text_file_backend >
            (
            boost::log::keywords::file_name = formatted_filename, 
            boost::log::keywords::rotation_size = 10 * 1024 * 1024,
            // rotate at midnight every day
            boost::log::keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
            // log collector,
            // -- maximum total size of the stored log files is 1GB.
            // -- minimum free space on the drive is 2GB
            boost::log::keywords::max_size = 1000 * 1024 * 1024,
            boost::log::keywords::min_free_space = 2000 * 1024 * 1024,
            boost::log::keywords::auto_flush = true
            //keywords::time_based_rotation = &is_it_time_to_rotate
        );
    typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t(backend));
    
    // specify the format of the log message 
    sink->set_formatter(&init_log_formatter<tag_file>);

    switch (op)
    {
        case log_op::operation::EQUAL :
            sink->set_filter(severity == threshold);
            break;
            
        case log_op::operation::GREATER_THAN :
            sink->set_filter(severity > threshold);
            break;
            
        case log_op::operation::GREATER_EQ_THAN :
            sink->set_filter(severity >= threshold);
            break;
            
        case log_op::operation::LESS_THAN :
            sink->set_filter(severity < threshold);
            break;
            
        case log_op::operation::LESS_EQ_THAN :
            sink->set_filter(severity <= threshold);
            break;
        
        default:
            break;
    }
    logging::core::get()->add_sink(sink);
}

// temporary : to be replaced with c++11 lambda
void set_global_log_level(log_op::operation op, custom_severity_level threshold )
{
    switch (threshold)
    {
        case custom_severity_level::TRACE :
            set_global_log_level_operation(op,custom_severity_level::TRACE);
            break;
            
        case custom_severity_level::DEBUG :
            set_global_log_level_operation(op,custom_severity_level::DEBUG);
            break;
            
        case custom_severity_level::RESULTS :
            set_global_log_level_operation(op,custom_severity_level::RESULTS);
            break;
            
        case custom_severity_level::INFO :
            set_global_log_level_operation(op,custom_severity_level::INFO);
            break;
            
        case custom_severity_level::WARN :
            set_global_log_level_operation(op,custom_severity_level::WARN);
            break;
        
        case custom_severity_level::STATE :
            set_global_log_level_operation(op,custom_severity_level::STATE);
            break;
            
        case custom_severity_level::ERROR :
            set_global_log_level_operation(op,custom_severity_level::ERROR);
            break;
            
        case custom_severity_level::NOLOG :
            set_global_log_level_operation(op,custom_severity_level::NOLOG);
            break;
            
        default:
            break;
    }
}

void set_global_log_level_operation(log_op::operation op, custom_severity_level threshold )
{
    switch (op)
    {
        case log_op::operation::EQUAL :
            boost::log::core::get()->set_filter(severity == threshold);
            break;
            
        case log_op::operation::GREATER_THAN :
            boost::log::core::get()->set_filter(severity > threshold);
            break;
            
        case log_op::operation::GREATER_EQ_THAN :
            boost::log::core::get()->set_filter(severity >= threshold);
            break;
            
        case log_op::operation::LESS_THAN :
            boost::log::core::get()->set_filter(severity < threshold);
            break;
            
        case log_op::operation::LESS_EQ_THAN :
            boost::log::core::get()->set_filter(severity <= threshold);
            break;
        
        default:
            break;
    }
}

void init_new_file(const std::string& filename, custom_severity_level threshold, log_op::operation op)
{
    // add a file text sink with filters but without any formatting
    std::string formatted_filename(filename);
    formatted_filename+=".%N.txt";
    boost::shared_ptr< sinks::text_file_backend > backend =
        boost::make_shared< sinks::text_file_backend >
            (
            boost::log::keywords::file_name = formatted_filename, 
            boost::log::keywords::rotation_size = 10 * 1024 * 1024,
            // rotate at midnight every day
            boost::log::keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
            // log collector,
            // -- maximum total size of the stored log files is 1GB.
            // -- minimum free space on the drive is 2GB
            boost::log::keywords::max_size = 1000 * 1024 * 1024,
            boost::log::keywords::min_free_space = 2000 * 1024 * 1024,
            boost::log::keywords::auto_flush = true
            //keywords::time_based_rotation = &is_it_time_to_rotate
        );
    typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t(backend));

    //sink->set_formatter(&init_file_formatter);
    
    switch (op)
    {
        case log_op::operation::EQUAL :
            sink->set_filter(severity == threshold);
            break;
            
        case log_op::operation::GREATER_THAN :
            sink->set_filter(severity > threshold);
            break;
            
        case log_op::operation::GREATER_EQ_THAN :
            sink->set_filter(severity >= threshold);
            break;
            
        case log_op::operation::LESS_THAN :
            sink->set_filter(severity < threshold);
            break;
            
        case log_op::operation::LESS_EQ_THAN :
            sink->set_filter(severity <= threshold);
            break;
        
        default:
            break;
    }
    logging::core::get()->add_sink(sink);
}

