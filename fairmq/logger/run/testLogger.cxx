/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

// WARNING : pragma commands to hide boost warning
// TODO : remove these pragma commands when boost will fix this issue in future release

#if defined(__clang__)
    _Pragma("clang diagnostic push") 
    _Pragma("clang diagnostic ignored \"-Wshadow\"") 
    #include "logger.h"
    _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
    _Pragma("GCC diagnostic push")
    _Pragma("GCC diagnostic ignored \"-Wshadow\"")
    #include "logger.h"
    _Pragma("GCC diagnostic pop")
#endif

#include <boost/log/support/date_time.hpp>

void test_logger()
{
    LOG(TRACE) << "this is a trace message";
    LOG(DEBUG) << "this is a debug message";
    LOG(RESULTS) << "this is a results message";
    LOG(INFO) << "this is a info message";
    LOG(WARN) << "this is a warning message";
    LOG(ERROR) << "this is an error message";
    LOG(STATE) << "this is a state message";
}

void test_console_level()
{
    std::cout<<"********* test logger : SET_LOG_CONSOLE_LEVEL(lvl) *********"<<std::endl;
    SET_LOG_CONSOLE_LEVEL(TRACE);
    test_logger();
    std::cout << "----------------------------"<<std::endl;

    SET_LOG_CONSOLE_LEVEL(DEBUG);
    test_logger();
    std::cout << "----------------------------"<<std::endl;

    SET_LOG_CONSOLE_LEVEL(RESULTS);
    test_logger();
    std::cout << "----------------------------"<<std::endl;

    SET_LOG_CONSOLE_LEVEL(INFO);
    test_logger();
    std::cout << "----------------------------"<<std::endl;

    SET_LOG_CONSOLE_LEVEL(WARN);
    test_logger();
    std::cout << "----------------------------"<<std::endl;

    SET_LOG_CONSOLE_LEVEL(ERROR);
    test_logger();
    std::cout << "----------------------------"<<std::endl;

    SET_LOG_CONSOLE_LEVEL(STATE);
    test_logger();
    std::cout << "----------------------------"<<std::endl;
}

int main()
{
    test_console_level();
    SET_LOG_CONSOLE_LEVEL(INFO);

    std::cout << "----------------------------"<<std::endl;
    LOG(INFO)<<"open log file 1";
    ADD_LOG_FILESINK("test_log1",ERROR);
    test_logger();

    std::cout << "----------------------------"<<std::endl;
    LOG(INFO)<<"open log file 2";
    ADD_LOG_FILESINK("test_log2",STATE);
    test_logger();

    // advanced commands
    std::cout << "----------------------------"<<std::endl;
    LOG(INFO)<<"open log file 3";// custom file sink setting
    AddFileSink([](const boost::log::attribute_value_set& attr_set)
        {
            auto sev = attr_set["Severity"].extract<custom_severity_level>();
            return (sev == FairMQ::ERROR);
        },
        boost::log::keywords::file_name = "test_log3_%5N.log",
        boost::log::keywords::rotation_size = 5 * 1024 * 1024,
        boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(12, 0, 0)
        );

    test_logger();

    std::cout << "----------------------------"<<std::endl;
    LOG(INFO)<<"set filter of last sink";// custom file sink setting
    // get last added sink and reset filter to WARN and ERROR
    FairMQ::Logger::sinkList.back()->set_filter([](const boost::log::attribute_value_set& attr_set)
        {
            auto sev = attr_set["Severity"].extract<custom_severity_level>();
            return (sev == FairMQ::WARN) || (sev == FairMQ::ERROR);
        });
    test_logger();

    // remove all sinks, and restart console sinks
    reinit_logger(false);
    test_logger();
    return 0;
}
