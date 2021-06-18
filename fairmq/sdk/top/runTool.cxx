/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <fairmq/Version.h>
#include <fairmq/sdk/top/Tool.h>

auto main(int argc, char** argv) -> int
{
    CLI::App app("htop-like TUI to monitor/control a running FairMQ topology\n", "fairmq-top");

    auto session(app.add_option("-s,--session", "DDS session id")
                     ->default_val(fair::mq::sdk::DDSSession::Id())
                     ->envname("DDS_SESSION_ID"));
    auto topo(app.add_option("-t,--topology", "DDS topology file")->check(CLI::ExistingFile));
    auto version(app.add_flag("-v,--version", "Print version")->excludes(session)->excludes(topo));

    try {
        app.parse(argc, argv);
    } catch (CLI::ParseError const& e) {
        return app.exit(e);
    }

    if (version->as<bool>()) {
        std::cout << FAIRMQ_GIT_VERSION << '\n';
        return EXIT_SUCCESS;
    }

    fair::mq::sdk::top::Tool tool(session->as<fair::mq::sdk::DDSSession::Id>());

    return tool.Run(topo->as<fair::mq::sdk::DDSTopology::Path>());
}
