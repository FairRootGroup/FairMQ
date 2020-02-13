/********************************************************************************
 *  Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/sdk/commands/Commands.h>
#include <fairmq/States.h>
#include <fairmq/SDK.h>

#include <boost/program_options.hpp>

#include <termios.h> // raw mode console input
#include <unistd.h>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

using namespace std;
using namespace fair::mq;
using namespace fair::mq::sdk;
using namespace fair::mq::sdk::cmd;
namespace bpo = boost::program_options;

struct TerminalConfig
{
    explicit TerminalConfig()
    {
        termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag &= ~ICANON; // disable canonical input
        // t.c_lflag &= ~ECHO; // do not echo input chars
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    }

    ~TerminalConfig()
    {
        termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag |= ICANON; // re-enable canonical input
        // t.c_lflag |= ECHO; // echo input chars
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    }
};

void printControlsHelp()
{
    cout << "Use keys to control the devices:" << endl;
    cout << "[c] check states, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [k] complete init, [b] bind, [x] connect" << endl;
    cout << "To quit press Ctrl+C" << endl;
}

void sendCommand(const string& commandIn, const string& topologyPath, Topology& topo)
{
    char c;
    string command(commandIn);
    TerminalConfig tconfig;

    if (command == "") {
        printControlsHelp();
        cin >> c;
        command = c;
    }

    while (true) {
        if (command == "c") {
            cout << "> checking state of the devices" << endl;
            topo.GetCurrentState();
            // TODO: extend me
        } else if (command == "o") {
            cout << "> dumping config of the devices" << endl;
            auto const result = topo.GetProperties("^(session|id)$", topologyPath);
            // TODO: extend me
        } else if (command == "i") {
            cout << "> init devices" << endl;
            topo.ChangeState(TopologyTransition::InitDevice);
        } else if (command == "k") {
            cout << "> complete init" << endl;
            topo.ChangeState(TopologyTransition::CompleteInit);
        } else if (command == "b") {
            cout << "> bind devices" << endl;
            topo.ChangeState(TopologyTransition::Bind);
        } else if (command == "x") {
            cout << "> connect devices" << endl;
            topo.ChangeState(TopologyTransition::Connect);
        } else if (command == "j") {
            cout << "> init tasks" << endl;
            topo.ChangeState(TopologyTransition::InitTask);
        } else if (command == "r") {
            cout << "> run tasks" << endl;
            topo.ChangeState(TopologyTransition::Run);
        } else if (command == "s") {
            cout << "> stop devices" << endl;
            topo.ChangeState(TopologyTransition::Stop);
        } else if (command == "t") {
            cout << "> reset tasks" << endl;
            topo.ChangeState(TopologyTransition::ResetTask);
        } else if (command == "d") {
            cout << "> reset devices" << endl;
            topo.ChangeState(TopologyTransition::ResetDevice);
        } else if (command == "h") {
            cout << "> help" << endl;
            printControlsHelp();
        } else if (command == "q") {
            cout << "> end" << endl;
            topo.ChangeState(TopologyTransition::End);
            // TODO: extend me..?
        } else {
            cout << "\033[01;32mInvalid input: [" << c << "]\033[0m" << endl;
            printControlsHelp();
        }

        if (commandIn != "") {
            this_thread::sleep_for(chrono::milliseconds(100)); // give dds a chance to complete request
            break;
        } else {
            cin >> c;
            command = c;
        }
    }
}

int main(int argc, char* argv[])
try {
    string topoFile;
    string sessionID;
    string command;
    string topologyPath;
    string targetState;
    unsigned int timeout;

    bpo::options_description opts("Common options");

    auto envSessionId = getenv("FAIRMQ_DDS_SESSION_ID");
    if (envSessionId) {
        opts.add_options()("session,s", bpo::value<string>(&sessionID)->default_value(envSessionId), "DDS Session ID (overrides any value in env var $FAIRMQ_DDS_SESSION_ID)");
    } else {
        opts.add_options()("session,s", bpo::value<string>(&sessionID)->required(), "DDS Session ID (overrides any value in env var $FAIRMQ_DDS_SESSION_ID)");
    }

    auto envTopoFile = getenv("FAIRMQ_DDS_TOPO_FILE");
    if (envTopoFile) {
        opts.add_options()("topology-file,f", bpo::value<string>(&topoFile)->default_value(envTopoFile), "DDS topology file path");
    } else {
        opts.add_options()("topology-file,f", bpo::value<string>(&topoFile)->required(), "DDS topology file path");
    }

    opts.add_options()
        ("command,c",        bpo::value<string>(&command)->default_value(""), "Command character")
        ("path,p",           bpo::value<string>(&topologyPath)->default_value(""), "DDS Topology path to send command to (empty - send to all tasks)")
        ("wait-for-state,w", bpo::value<string>(&targetState)->default_value(""), "Wait until targeted FairMQ devices reach the given state")
        ("timeout,t",        bpo::value<unsigned int>(&timeout)->default_value(0), "Timeout in milliseconds when waiting for a device state (0 - wait infinitely)")
        ("help,h",           "Produce help message");

    bpo::variables_map vm;
    bpo::store(bpo::command_line_parser(argc, argv).options(opts).run(), vm);

    if (vm.count("help")) {
        cout << "FairMQ DDS Command UI" << endl << opts << endl;
        cout << "Commands: [c] check state, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [k] complete init, [b] bind, [x] connect" << endl;
        return EXIT_SUCCESS;
    }

    bpo::notify(vm);

    DDSEnvironment env;
    DDSSession session(sessionID, env);
    DDSTopology ddsTopo(DDSTopology::Path(topoFile), env);

    int n = ddsTopo.GetNumRequiredAgents();
    cout << "Number of required agents/slots: " << n << endl;
    cout << "creating Topology" << endl;

    Topology topo(ddsTopo, session);
    for (auto transition : { TopologyTransition::InitDevice,
                             TopologyTransition::CompleteInit,
                             TopologyTransition::Bind,
                             TopologyTransition::Connect,
                             TopologyTransition::InitTask,
                             TopologyTransition::Run,
                             TopologyTransition::Stop,
                             TopologyTransition::ResetTask,
                             TopologyTransition::ResetDevice,
                             TopologyTransition::End }) {
        topo.ChangeState(transition);
    }

    cout << "Finishing..." << endl;
    return EXIT_SUCCESS;
} catch (exception& e) {
    cerr << "Error: " << e.what() << endl;
    return EXIT_FAILURE;
}
