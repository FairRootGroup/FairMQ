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
    cout << "[c] check states, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [k] complete init, [b] bind, [x] connect, [p] set property" << endl;
    cout << "To quit press Ctrl+C" << endl;
}

void handleCommand(const string& command, const string& path, unsigned int timeout, Topology& topo, const string& pKey, const string& pVal)
{
    std::pair<std::error_code, fair::mq::sdk::TopologyState> changeStateResult;

    if (command == "c") {
        cout << "> checking state of the devices" << endl;
        auto const result = topo.GetCurrentState();
        for (const auto& d : result) {
            cout << d.taskId << " : " << d.state << endl;
        }
        return;
    } else if (command == "o") {
        cout << "> dumping config of " << (path == "" ? "all" : path) << endl;
        // TODO: extend this regex to return all properties, once command size limitation is removed.
        auto const result = topo.GetProperties("^(session|id)$", path, std::chrono::milliseconds(timeout));
        for (const auto& d : result.second.devices) {
            for (auto const& p : d.second.props) {
                cout << d.first << ": " << p.first << " : " << p.second << endl;
            }
        }
        return;
    } else if (command == "p") {
        if (pKey == "" || pVal == "") {
            cout << "cannot send property with empty key and/or value! given key: '" << pKey << "', value: '" << pVal << "'." << endl;
            return;
        }
        const DeviceProperties props{{pKey, pVal}};
        cout << "> setting properties --> " << (path == "" ? "all" : path) << endl;
        topo.SetProperties(props, path);
        // give dds time to complete request
        this_thread::sleep_for(chrono::milliseconds(100));
        return;
    } else if (command == "i") {
        cout << "> initiating InitDevice transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::InitDevice, path, std::chrono::milliseconds(timeout));
    } else if (command == "k") {
        cout << "> initiating CompleteInit transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::CompleteInit, path, std::chrono::milliseconds(timeout));
    } else if (command == "b") {
        cout << "> initiating Bind transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::Bind, path, std::chrono::milliseconds(timeout));
    } else if (command == "x") {
        cout << "> initiating Connect transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::Connect, path, std::chrono::milliseconds(timeout));
    } else if (command == "j") {
        cout << "> initiating InitTask transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::InitTask, path, std::chrono::milliseconds(timeout));
    } else if (command == "r") {
        cout << "> initiating Run transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::Run, path, std::chrono::milliseconds(timeout));
    } else if (command == "s") {
        cout << "> initiating Stop transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::Stop, path, std::chrono::milliseconds(timeout));
    } else if (command == "t") {
        cout << "> initiating ResetTask transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::ResetTask, path, std::chrono::milliseconds(timeout));
    } else if (command == "d") {
        cout << "> initiating ResetDevice transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::ResetDevice, path, std::chrono::milliseconds(timeout));
    } else if (command == "q") {
        cout << "> initiating End transition --> " << (path == "" ? "all" : path) << endl;
        changeStateResult = topo.ChangeState(TopologyTransition::End, path, std::chrono::milliseconds(timeout));
    } else if (command == "h") {
        cout << "> help" << endl;
        printControlsHelp();
        return;
    } else {
        cout << "\033[01;32mInvalid input: [" << command << "]\033[0m" << endl;
        printControlsHelp();
        return;
    }
    if (changeStateResult.first != std::error_code()) {
        cout << "ERROR: ChangeState failed for '" << path << "': " << changeStateResult.first.message() << endl;
    }
}

void sendCommand(const string& commandIn, const string& path, unsigned int timeout, Topology& topo, const string& pKey, const string& pVal)
{
    if (commandIn != "") {
        handleCommand(commandIn, path, timeout, topo, pKey, pVal);
        return;
    }

    char c;
    string command;
    TerminalConfig tconfig;

    printControlsHelp();
    cin >> c;
    command = c;

    while (true) {
        handleCommand(command, path, timeout, topo, pKey, pVal);
        cin >> c;
        command = c;
    }
}

int main(int argc, char* argv[])
try {
    string sessionID;
    string topoFile;

    string command;
    string path;
    string targetState;
    string pKey;
    string pVal;
    unsigned int timeout;

    fair::Logger::SetConsoleSeverity("debug");
    fair::Logger::SetConsoleColor(true);

    bpo::options_description options("Common options");

    auto envSessionId = getenv("DDS_SESSION_ID");
    if (envSessionId) {
        options.add_options()("session,s", bpo::value<string>(&sessionID)->default_value(envSessionId), "DDS Session ID (overrides any value in env var $DDS_SESSION_ID)");
    } else {
        options.add_options()("session,s", bpo::value<string>(&sessionID)->required(), "DDS Session ID (overrides any value in env var $DDS_SESSION_ID)");
    }

    auto envTopoFile = getenv("FAIRMQ_DDS_TOPO_FILE");
    if (envTopoFile) {
        options.add_options()("topology-file,f", bpo::value<string>(&topoFile)->default_value(envTopoFile), "DDS topology file path");
    } else {
        options.add_options()("topology-file,f", bpo::value<string>(&topoFile)->required(), "DDS topology file path");
    }

    options.add_options()
        ("command,c",        bpo::value<string>(&command)->default_value(""), "Command character")
        ("path,p",           bpo::value<string>(&path)->default_value(""), "DDS Topology path to send command to (empty - send to all tasks)")
        ("property-key",     bpo::value<string>(&pKey)->default_value(""), "property key to be used with 'p' command")
        ("property-value",   bpo::value<string>(&pVal)->default_value(""), "property value to be used with 'p' command")
        ("wait-for-state,w", bpo::value<string>(&targetState)->default_value(""), "Wait until targeted FairMQ devices reach the given state")
        ("timeout,t",        bpo::value<unsigned int>(&timeout)->default_value(0), "Timeout in milliseconds when waiting for a device state (0 - wait infinitely)")
        ("help,h",           "Produce help message");

    bpo::variables_map vm;
    bpo::store(bpo::command_line_parser(argc, argv).options(options).run(), vm);

    if (vm.count("help")) {
        cout << "FairMQ DDS Command UI" << endl << options << endl;
        cout << "Commands: [c] check state, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [k] complete init, [b] bind, [x] connect, [p] set property" << endl;
        return EXIT_SUCCESS;
    }

    bpo::notify(vm);

    DDSEnvironment env;
    DDSSession session(sessionID, env);
    DDSTopology ddsTopo(DDSTopology::Path(topoFile), env);

    Topology topo(ddsTopo, session);

    if (targetState != "") {
        if (command != "") {
            sendCommand(command, path, timeout, topo, pKey, pVal);
        }
        size_t pos = targetState.find("->");
        cout << "> waiting for " << (path == "" ? "all" : path) << " to reach " << targetState << endl;
        if (pos == string::npos) {
            /* auto ec =  */topo.WaitForState(GetState(targetState), path, std::chrono::milliseconds(timeout));
            // cout << "WaitForState(" << targetState << ") result: " << ec.message() << endl;
        } else {
            /* auto ec =  */topo.WaitForState(GetState(targetState.substr(0, pos)), GetState(targetState.substr(pos + 2)), path, std::chrono::milliseconds(timeout));
            // cout << "WaitForState(" << targetState << ") result: " << ec.message() << endl;
        }
    } else {
        sendCommand(command, path, timeout, topo, pKey, pVal);
    }

    return EXIT_SUCCESS;
} catch (exception& e) {
    cerr << "Error: " << e.what() << endl;
    return EXIT_FAILURE;
}
