/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <asio/associated_executor.hpp>
#include <asio/executor.hpp>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asio/use_future.hpp>
#include <boost/program_options.hpp>
#include <fairmq/sdk/top/Top.h>
#include <fairmq/sdk/DDSSession.h>
#include <fairmq/Version.h>
#include <future>
#include <iostream>
#include <thread>

namespace {

auto printHelp(const boost::program_options::options_description& options) -> void
{
    std::cout << "Usage:" << std::endl;
    std::cout << "  fairmq-top [-s <session id>] [-h]" << std::endl;
    std::cout << std::endl;
    std::cout << "  htop-like monitor for a running FairMQ topology" << std::endl;
    std::cout << std::endl;
    std::cout << options;
    std::cout << std::endl;
    std::cout << "FairMQ v" << FAIRMQ_GIT_VERSION << " ©" << FAIRMQ_COPYRIGHT <<std::endl;
}

template<typename Executor>
auto makeTop(Executor ex, fair::mq::sdk::DDSSession::Id sessionId) -> fair::mq::sdk::Top
{
    using namespace fair::mq;

    if (sessionId.empty()) {
        return sdk::Top(std::move(ex), sdk::getMostRecentRunningDDSSession());
    }
    return sdk::Top(std::move(ex), sdk::DDSSession(sessionId));
}

struct NoCopy
{
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;

    NoCopy() = default;
    NoCopy(NoCopy&&) = default;
    NoCopy& operator=(NoCopy&&) = default;
    ~NoCopy() = default;
};

} // anonymous namespace

int main(int argc, char const* argv[])
{
    using namespace fair::mq;

    sdk::DDSSession::Id sessionId;
    auto sessionOptionValue(boost::program_options::value<sdk::DDSSession::Id>(&sessionId));
    auto envSessionId = getenv("DDS_SESSION_ID");
    if (envSessionId != nullptr) {
        sessionOptionValue->default_value(sdk::DDSSession::Id(envSessionId));
    }

    boost::program_options::options_description options("Options");
    options.add_options()
        ("session,s", sessionOptionValue,
                      "DDS Session ID (overrides $DDS_SESSION_ID). If no session id is "
                      "provided it tries to find most recent running session.")
        ("help,h", "Print usage help");

    try {
        boost::program_options::variables_map vm;
        boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv).options(options).run(), vm);
        boost::program_options::notify(vm);

        if (vm.count("help") > 0) {
            printHelp(options);
            return EXIT_SUCCESS;
        }

        // Execution context
        asio::io_context context;

        // Abort on SIGINT and SIGTERM
        asio::signal_set signals(context.get_executor(), SIGINT, SIGTERM);
        signals.async_wait([&](std::error_code, int) { context.stop(); });

        // Run fairmq-top
        auto app = makeTop(context.get_executor(), sessionId);
        int rc(EXIT_SUCCESS);

        ///// move-only lambda
        //
        NoCopy noCopy; // just to check completion lambda is never copied
        app.AsyncRun([&, nc=std::move(noCopy)](std::error_code ec) {
            if (ec) {
                rc = EXIT_FAILURE;
            } else {
                rc = EXIT_SUCCESS;
            }
            context.stop();
        });

        context.run();
        //
        /////////////////////

        /// OR

        ///// std::future
        //
        // std::thread ctxThread([&]{ context.run(); });
//
        // std::future<void> future = app.AsyncRun(asio::use_future);
//
        // try {
        // future.get();
        // rc = EXIT_SUCCESS;
        // } catch (const system::error_code& e) {
        // rc = EXIT_FAILURE;
        // }
//
        // context.stop();
        // ctxThread.join();
        //
        /////////////////////

        return rc;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
