/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tools/Process.h>

#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <sstream>
#include <thread>
#include <stdexcept>

using namespace std;
namespace bp = boost::process;
namespace ba = boost::asio;

namespace fair
{
namespace mq
{
namespace tools
{

/**
 * Execute given command in forked process and capture stdout output
 * and exit code.
 *
 * @param[in] cmd Command to execute
 * @param[in] log_prefix How to prefix each captured output line with
 * @return Captured stdout output and exit code
 */
execute_result execute(const string& cmd, const string& prefix, const string& input)
{
    execute_result result;
    stringstream out;

    // print full line thread-safe
    stringstream printCmd;
    printCmd << prefix << " " << cmd << "\n";
    cout << printCmd.str() << flush;
    out << prefix << cmd << endl;

    ba::io_service ios;

    // containers for std_in
    ba::const_buffer inputBuffer(ba::buffer(input));
    bp::async_pipe inputPipe(ios);
    // containers for std_out
    ba::streambuf outputBuffer;
    bp::async_pipe outputPipe(ios);
    // containers for std_err
    ba::streambuf errorBuffer;
    bp::async_pipe errorPipe(ios);

    const string delimiter = "\n";
    ba::deadline_timer timer(ios, boost::posix_time::milliseconds(100));

    // child process
    bp::child c(cmd, bp::std_out > outputPipe, bp::std_err > errorPipe, bp::std_in < inputPipe, ios);

    // handle std_in with a delay
    if (input != "") {
        timer.async_wait([&](const boost::system::error_code& ec1) {
            if (!ec1) {
                ba::async_write(inputPipe, inputBuffer, [&](const boost::system::error_code& ec2, size_t /* n */) {
                    if (!ec2) {
                        inputPipe.async_close();
                    } else {
                        out << prefix << "error in boost::asio::async_write: " << ec2.value() << endl;
                    }
                });
            } else {
                out << prefix << "error in boost::asio::deadline_timer.async_wait: " << ec1.value() << endl;
            }
        });
    }

    // handle std_out line by line
    function<void(const boost::system::error_code&, size_t)> onStdOut = [&](const boost::system::error_code& ec, size_t /* n */) {
        if (!ec) {
            istream is(&outputBuffer);
            string line;
            getline(is, line);

            stringstream printLine;
            printLine << prefix << line << "\n";
            cout << printLine.str() << flush;
            out << prefix << line << endl;

            ba::async_read_until(outputPipe, outputBuffer, delimiter, onStdOut);
        } else {
            outputPipe.async_close();
        }
    };
    ba::async_read_until(outputPipe, outputBuffer, delimiter, onStdOut);

    // handle std_err line by line
    function<void(const boost::system::error_code&, size_t)> onStdErr = [&](const boost::system::error_code& ec, size_t /* n */) {
        if (!ec) {
            istream is(&errorBuffer);
            string line;
            getline(is, line);

            stringstream printLine;
            printLine << prefix << line << "\n";
            cerr << printLine.str() << flush;
            out << prefix << "error: " << line << endl;

            ba::async_read_until(errorPipe, errorBuffer, delimiter, onStdErr);
        } else {
            errorPipe.async_close();
        }
    };
    ba::async_read_until(errorPipe, errorBuffer, delimiter, onStdErr);

    ios.run();
    c.wait();

    result.exit_code = c.exit_code();
    stringstream exitCode;

    exitCode << prefix << " Exit code: " << result.exit_code << "\n";
    cout << exitCode.str() << flush;
    out << prefix << " Exit code: " << result.exit_code << endl;
    result.console_out = out.str();

    return result;
}

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */
