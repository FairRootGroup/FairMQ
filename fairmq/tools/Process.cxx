/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tools/Process.h>
#include <fairmq/tools/Strings.h>

#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <signal.h> // kill, signals

#include <iostream>
#include <sstream>
#include <thread>
#include <stdexcept>

using namespace std;
namespace bp = boost::process;
namespace ba = boost::asio;
namespace bs = boost::system;

class LinePrinter
{
  public:
    LinePrinter(stringstream& out, const string& prefix)
        : fOut(out)
        , fPrefix(prefix)
    {}

    // prints line with prefix on both cout (thread-safe) and output stream
    void Print(const string& line)
    {
        cout << fair::mq::tools::ToString(fPrefix, line, "\n") << flush;
        fOut << fPrefix << line << endl;
    }

  private:
    stringstream& fOut;
    const string fPrefix;
};

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
execute_result execute(const string& cmd, const string& prefix, const string& input, int sig)
{
    execute_result result;
    stringstream out;

    LinePrinter p(out, prefix);

    p.Print(cmd);

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
    ba::deadline_timer inputTimer(ios, boost::posix_time::milliseconds(100));
    ba::deadline_timer signalTimer(ios, boost::posix_time::milliseconds(100));

    // child process
    bp::child c(cmd, bp::std_out > outputPipe, bp::std_err > errorPipe, bp::std_in < inputPipe);
    int pid = c.id();
    p.Print(ToString("fair::mq::tools::execute: pid: ", pid));

    // handle std_in with a delay
    if (input != "") {
        inputTimer.async_wait([&](const bs::error_code& ec1) {
            if (!ec1) {
                ba::async_write(inputPipe, inputBuffer, [&](const bs::error_code& ec2, size_t /* n */) {
                    if (!ec2) {
                        // inputPipe.async_close();
                    } else {
                        p.Print(ToString("error in boost::asio::async_write: ", ec2.message()));
                    }
                });
            } else {
                p.Print(ToString("error in boost::asio::deadline_timer.async_wait: ", ec1.message()));
            }
        });
    }

    if (sig != -1) {
        signalTimer.async_wait([&](const bs::error_code& ec1) {
            if (!ec1) {
                kill(pid, sig);
            } else {
                p.Print(ToString("error in boost::asio::deadline_timer.async_wait: ", ec1.message()));
            }
        });
    }

    // handle std_out line by line
    function<void(const bs::error_code&, size_t)> onStdOut = [&](const bs::error_code& ec, size_t /* n */) {
        if (!ec) {
            istream is(&outputBuffer);
            string line;
            getline(is, line);

            p.Print(line);

            ba::async_read_until(outputPipe, outputBuffer, delimiter, onStdOut);
        } else {
            if (ec == ba::error::eof) {
                // outputPipe.async_close();
            } else {
                p.Print(ec.message());
            }
        }
    };
    ba::async_read_until(outputPipe, outputBuffer, delimiter, onStdOut);

    // handle std_err line by line
    function<void(const bs::error_code&, size_t)> onStdErr = [&](const bs::error_code& ec, size_t /* n */) {
        if (!ec) {
            istream is(&errorBuffer);
            string line;
            getline(is, line);

            p.Print(ToString("error: ", line));

            ba::async_read_until(errorPipe, errorBuffer, delimiter, onStdErr);
        } else {
            if (ec == ba::error::eof) {
                // errorPipe.async_close();
            } else {
                p.Print(ec.message());
            }
        }
    };
    ba::async_read_until(errorPipe, errorBuffer, delimiter, onStdErr);

    ios.run();
    c.wait();

    result.exit_code = c.exit_code();

    p.Print(ToString("fair::mq::tools::execute: exit code: ", result.exit_code));
    result.console_out = out.str();

    return result;
}

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */
