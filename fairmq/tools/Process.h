/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_PROCESS_H
#define FAIR_MQ_TOOLS_PROCESS_H

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
 * @param[in] prefix How to prefix each captured output line with
 * @param[in] input Data which is sent to stdin of the child process
 * @return Captured stdout output and exit code
 */
execute_result execute(const std::string& cmd,
                       const std::string& prefix = "",
                       const std::string& input = "",
                       int sig = -1);

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TOOLS_PROCESS_H */
