/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_RUNNER_H
#define FAIR_MQ_TEST_RUNNER_H

#include <string>

namespace fair
{
namespace mq
{
namespace test
{

extern std::string runTestDevice; /// Path to test device executable.
extern std::string mqConfig; /// Path to FairMQ device config file.

/**
 * Result type for execute function. Holds captured stderr output and exit code.
 */
struct execute_result {
    std::string error_out;
    int exit_code;
};

/**
 * Execute given command in forked process and capture stderr output
 * and exit code.
 *
 * @param[in] cmd Command to execute
 * @param[in] log_prefix How to prefix each captured output line with
 * @return Captured error output and exit code
 */
auto execute(std::string cmd, std::string log_prefix = "") -> execute_result;

} /* namespace test */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TEST_RUNNER_H */
