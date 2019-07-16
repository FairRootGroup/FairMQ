/********************************************************************************
 *  Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <algorithm>
#include <atomic>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/program_options.hpp>
#include <condition_variable>
#include <cstdlib>
#include <DDS/dds_intercom.h>
#include <exception>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <termios.h>   // raw mode console input
#include <thread>
#include <string>
#include <unistd.h>

using namespace std;
using namespace dds::intercom_api;
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

struct StateSubscription {
    const string& fTopologyPath;
    CCustomCmd& fDdsCustomCmd;

    explicit StateSubscription(const string& topologyPath, CCustomCmd& ddsCustomCmd)
        : fTopologyPath(topologyPath)
        , fDdsCustomCmd(ddsCustomCmd)
    {
        fDdsCustomCmd.send("subscribe-to-state-changes", fTopologyPath);
    }

    ~StateSubscription() {
        fDdsCustomCmd.send("unsubscribe-from-state-changes", fTopologyPath);
        this_thread::sleep_for(chrono::milliseconds(100)); // give dds a chance to complete request
    }
};

void printControlsHelp()
{
    cout << "Use keys to control the devices:" << endl;
    cout << "[c] check states, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [b] bind, [x] connect" << endl;
    cout << "To quit press Ctrl+C" << endl;
}

void commandMode(const string& commandIn, const string& topologyPath, CCustomCmd& ddsCustomCmd) {
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
            ddsCustomCmd.send("check-state", topologyPath);
        } else if (command == "o") {
            cout << "> dumping config of the devices" << endl;
            ddsCustomCmd.send("dump-config", topologyPath);
        } else if (command == "i") {
            cout << "> init devices" << endl;
            ddsCustomCmd.send("INIT DEVICE", topologyPath);
        } else if (command == "b") {
            cout << "> bind devices" << endl;
            ddsCustomCmd.send("BIND", topologyPath);
        } else if (command == "x") {
            cout << "> connect devices" << endl;
            ddsCustomCmd.send("CONNECT", topologyPath);
        } else if (command == "j") {
            cout << "> init tasks" << endl;
            ddsCustomCmd.send("INIT TASK", topologyPath);
        } else if (command == "p") {
            cout << "> pause devices" << endl;
            ddsCustomCmd.send("PAUSE", topologyPath);
        } else if (command == "r") {
            cout << "> run tasks" << endl;
            ddsCustomCmd.send("RUN", topologyPath);
        } else if (command == "s") {
            cout << "> stop devices" << endl;
            ddsCustomCmd.send("STOP", topologyPath);
        } else if (command == "t") {
            cout << "> reset tasks" << endl;
            ddsCustomCmd.send("RESET TASK", topologyPath);
        } else if (command == "d") {
            cout << "> reset devices" << endl;
            ddsCustomCmd.send("RESET DEVICE", topologyPath);
        } else if (command == "h") {
            cout << "> help" << endl;
            printControlsHelp();
        } else if (command == "q") {
            cout << "> end" << endl;
            ddsCustomCmd.send("END", topologyPath);
        } else if (command == "q!") {
            cout << "> shutdown" << endl;
            ddsCustomCmd.send("SHUTDOWN", topologyPath);
        } else if (command == "r!") {
            cout << "> startup" << endl;
            ddsCustomCmd.send("STARTUP", topologyPath);
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

struct WaitMode
{
    explicit WaitMode(const string& targetState)
        : fTargetState(targetState)
    {}

    void Run(const chrono::milliseconds& timeout, const string& topologyPath, CCustomCmd& ddsCustomCmd, const string& command = "")
    {
        StateSubscription stateSubscription(topologyPath, ddsCustomCmd);

        if (command != "") {
            commandMode(command, topologyPath, ddsCustomCmd);
        }

        // TODO once DDS provides an API to retrieve actual number of tasks, use it here
        auto condition = [&] { return !fTargetStates.empty() && all_of(fTargetStates.cbegin(),
                                                                    fTargetStates.cend(),
                                                                    [&](unordered_map<uint64_t, string>::value_type i) {
                                                                        return boost::algorithm::ends_with(i.second, fTargetState);
                                                                    });
        };

        unique_lock<mutex> lock(fMtx);

        if (timeout > chrono::milliseconds(0)) {
            if (!fCV.wait_for(lock, timeout, condition)) {
                throw runtime_error("timeout");
            }
        } else {
            fCV.wait(lock, condition);
        }
    }

    void AddNewStateEntry(uint64_t senderId, const string& state)
    {
        {
            unique_lock<mutex> lock(fMtx);
            fTargetStates[senderId] = state;
        }
        fCV.notify_one();
    }

    mutex fMtx;
    condition_variable fCV;
    unordered_map<uint64_t, string> fTargetStates;
    string fTargetState;
};

int main(int argc, char* argv[])
{
    try {
        string sessionID;
        string command;
        string topologyPath;
        string targetState;
        unsigned int timeout;

        bpo::options_description options("Common options");

        auto envSessionId = getenv("DDS_SESSION_ID");
        if (envSessionId) {
            options.add_options()("session,s", bpo::value<string>(&sessionID)->default_value(envSessionId), "DDS Session ID (overrides any value in env var $DDS_SESSION_ID)");
        } else {
            options.add_options()("session,s", bpo::value<string>(&sessionID)->required(), "DDS Session ID (overrides any value in env var $DDS_SESSION_ID)");
        }

        options.add_options()
            ("command,c",        bpo::value<string> (&command)->default_value(""), "Command character")
            ("path,p",           bpo::value<string> (&topologyPath)->default_value(""), "DDS Topology path to send command to (empty - send to all tasks)")
            ("wait-for-state,w", bpo::value<string> (&targetState)->default_value(""), "Wait until targeted FairMQ devices reach the given state")
            ("timeout,t",        bpo::value<unsigned int> (&timeout)->default_value(0), "Timeout in milliseconds when waiting for a device state (0 - wait infinitely)")
            ("help,h", "Produce help message");

        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(options).run(), vm);

        if (vm.count("help")) {
            cout << "FairMQ DDS Command UI" << endl << options << endl;
            cout << "Commands: [c] check state, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device" << endl;
            return EXIT_SUCCESS;
        }

        bpo::notify(vm);

        WaitMode waitMode(targetState);

        CIntercomService service;
        CCustomCmd ddsCustomCmd(service);

        service.subscribeOnError([](const EErrorCode errorCode, const string& errorMsg) {
            cerr << "DDS error received: error code: " << errorCode << ", error message: " << errorMsg << endl;
        });

        // subscribe to receive messages from DDS
        ddsCustomCmd.subscribe([&](const string& msg, const string& /*condition*/, uint64_t senderId) {
            cerr << "Received: " << msg << endl;
            vector<string> parts;
            boost::algorithm::split(parts, msg, boost::algorithm::is_any_of(":,"));
            if (parts[0] == "state-change") {
                boost::trim(parts[2]);
                waitMode.AddNewStateEntry(senderId, parts[2]);
            } else if (parts[0] == "state-changes-subscription") {
                if (parts[2] != "OK") {
                    cerr << "state-changes-subscription failed with return code: " << parts[2];
                }
            } else if (parts[0] == "state-changes-unsubscription") {
                if (parts[2] != "OK") {
                    cerr << "state-changes-unsubscription failed with return code: " << parts[2];
                }
            } else {
                // cout << "Received: " << msg << endl;
            }
        });

        service.start(sessionID);

        if (targetState == "") {
            commandMode(command, topologyPath, ddsCustomCmd);
        } else {
            waitMode.Run(chrono::milliseconds(timeout), topologyPath, ddsCustomCmd, command);
        }
    } catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
