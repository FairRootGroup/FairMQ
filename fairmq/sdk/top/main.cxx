/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <boost/asio/executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <fairmq/sdk/top/Top.h>
#include <fairmq/sdk/DDSSession.h>
#include <fairmq/Version.h>
#include <iostream>

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

auto makeTop(boost::asio::executor executor, fair::mq::sdk::DDSSession::Id sessionId)
    -> fair::mq::sdk::Top
{
    if (sessionId.empty()) {
        return fair::mq::sdk::Top(std::move(executor),
                                  fair::mq::sdk::getMostRecentRunningDDSSession());
    } else {
        return fair::mq::sdk::Top(std::move(executor), fair::mq::sdk::DDSSession(sessionId));
    }
}

int main(int argc, char const* argv[])
{
    fair::mq::sdk::DDSSession::Id sessionId;

    auto sessionOptionValue(boost::program_options::value<fair::mq::sdk::DDSSession::Id>(&sessionId));
    auto envSessionId = getenv("DDS_SESSION_ID");
    if (envSessionId != nullptr) {
        sessionOptionValue->default_value(fair::mq::sdk::DDSSession::Id(envSessionId));
    }
    boost::program_options::options_description options("Options");
    options.add_options()
        ("session,s", sessionOptionValue, "DDS Session ID (overrides $DDS_SESSION_ID). If no session id is provided it tries to find most recent running session.")
        ("help,h", "Print usage help");

    boost::program_options::variables_map vm;

    try {
        boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv).options(options).run(), vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            printHelp(options);
            return EXIT_SUCCESS;
        }

        // Execution context
        boost::asio::io_context context;
        auto executor(context.get_executor());

        // Abort on SIGINT and SIGTERM
        boost::asio::signal_set signals(context, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code&, int) { context.stop(); });

        // Run fairmq-top
        auto app = makeTop(executor, sessionId);
        int rc(EXIT_SUCCESS);
        app.AsyncRun();

        // Run app single-threaded
        context.run();

        return rc;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
