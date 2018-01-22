/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_PROCESS_H
#define FAIR_MQ_TOOLS_PROCESS_H

#include <boost/process.hpp>

#include <string>

namespace fair
{
namespace mq
{
namespace tools
{

/**
 * Result type for execute function. Holds captured stdout output and exit code.
 */
struct execute_result
{
    std::string console_out;
    int exit_code;
};

/**
 * Execute given command in forked process and capture stdout output
 * and exit code.
 *
 * @param[in] cmd Command to execute
 * @param[in] log_prefix How to prefix each captured output line with
 * @return Captured stdout output and exit code
 */
inline execute_result execute(std::string cmd, std::string prefix = "")
{
    execute_result result;
    std::stringstream out;

    // print full line thread-safe
    std::stringstream printCmd;
    printCmd << prefix << cmd << "\n";
    std::cout << printCmd.str() << std::flush;

    out << prefix << cmd << std::endl;

    // Execute command and capture stdout, add prefix line by line
    boost::process::ipstream stdout;
    boost::process::child c(cmd, boost::process::std_out > stdout);
    std::string line;
    while (getline(stdout, line))
    {
        // print full line thread-safe
        std::stringstream printLine;
        printLine << prefix << line << "\n";
        std::cout << printLine.str() << std::flush;

        out << prefix << line << "\n";
    }

    c.wait();

    // Capture exit code
    result.exit_code = c.exit_code();
    out << prefix << " Exit code: " << result.exit_code << std::endl;

    result.console_out = out.str();

    // Return result
    return result;
}

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_PROCESS_H */
