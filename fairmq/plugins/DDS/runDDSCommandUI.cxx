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
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <condition_variable>
#include <dds_intercom.h>
#include <exception>
#include <iostream>
#include <map>
#include <mutex>
#include <termios.h>   // raw mode console input
#include <thread>
#include <string>
#include <unistd.h>

using namespace std;
using namespace dds::intercom_api;
namespace bpo = boost::program_options;
using WaitForStateMap = map<uint64_t, string>;

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
    cout << "[c] check states, [o] dump config, [h] help, [p] pause, [r] run, [s] stop, [t] reset "
            "task, [d] reset device, [q] end, [j] init task, [i] init device"
         << endl;
    cout << "To quit press Ctrl+C" << endl;
}

void commandMode(char command, const string& topologyPath, CCustomCmd& ddsCustomCmd) {
    char c;
    TerminalConfig tconfig;

    if (command != ' ') {
        cin.putback(command);
    } else {
        printControlsHelp();
    }

    while (cin >> c) {
        switch (c) {
            case 'c':
                cout << " > checking state of the devices" << endl;
                ddsCustomCmd.send("check-state", topologyPath);
                break;
            case 'o':
                cout << " > dumping config of the devices" << endl;
                ddsCustomCmd.send("dump-config", topologyPath);
                break;
            case 'i':
                cout << " > init devices" << endl;
                ddsCustomCmd.send("INIT DEVICE", topologyPath);
                break;
            case 'j':
                cout << " > init tasks" << endl;
                ddsCustomCmd.send("INIT TASK", topologyPath);
                break;
            case 'p':
                cout << " > pause devices" << endl;
                ddsCustomCmd.send("PAUSE", topologyPath);
                break;
            case 'r':
                cout << " > run tasks" << endl;
                ddsCustomCmd.send("RUN", topologyPath);
                break;
            case 's':
                cout << " > stop devices" << endl;
                ddsCustomCmd.send("STOP", topologyPath);
                break;
            case 't':
                cout << " > reset tasks" << endl;
                ddsCustomCmd.send("RESET TASK", topologyPath);
                break;
            case 'd':
                cout << " > reset devices" << endl;
                ddsCustomCmd.send("RESET DEVICE", topologyPath);
                break;
            case 'h':
                cout << " > help" << endl;
                printControlsHelp();
                break;
            case 'q':
                cout << " > end" << endl;
                ddsCustomCmd.send("END", topologyPath);
                break;
            default:
                cout << "Invalid input: [" << c << "]" << endl;
                printControlsHelp();
                break;
        }

        if (command != ' ') {
            this_thread::sleep_for(chrono::milliseconds(100)); // give dds a chance to complete request
            break;
        }
    }
}

void waitMode(const string& waitForState,
              mutex& waitForStateMutex,
              condition_variable& waitForStateCV,
              const WaitForStateMap& waitForStateMap,
              const string& topologyPath,
              CCustomCmd& ddsCustomCmd,
              chrono::milliseconds timeout)
{
    StateSubscription stateSubscription(topologyPath, ddsCustomCmd);

    auto condition = [&] {
        return all_of(waitForStateMap.cbegin(),
                      waitForStateMap.cend(),
                      [&](WaitForStateMap::value_type i) { return i.second == waitForState; });
    };

    unique_lock<mutex> lock(waitForStateMutex);
    if (!waitForStateCV.wait_for(lock, timeout, condition)) {
        throw runtime_error("timeout");
    };
}

int main(int argc, char* argv[])
{
    try {
        string sessionID;
        char command = ' ';
        string topologyPath;
        string waitForState;
        unsigned int timeout;
        mutex waitForStateMutex;
        condition_variable waitForStateCV;
        WaitForStateMap waitForStateMap;

        bpo::options_description options("Common options");
        options.add_options()
            ("session,s",        bpo::value<string> (&sessionID)->required(),
                                 "DDS Session ID")
            ("command,c",        bpo::value<char>   (&command)->default_value(' '),
                                 "Command character")
            ("path,p",           bpo::value<string> (&topologyPath)->default_value(""),
                                 "DDS Topology path to send command to")
            ("wait-for-state,w", bpo::value<string> (&waitForState)->default_value(""),
                                 "Wait until targeted FairMQ devices reach the given state")
            ("timeout,t",        bpo::value<unsigned int> (&timeout)->default_value(0),
                                 "Timeout in milliseconds when waiting for a device state (0 - wait infinitely)")

            ("help,h", "Produce help message");

        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(options).run(), vm);

        if (vm.count("help")) {
            cout << "FairMQ DDS Command UI" << endl << options << endl;
            cout << "Commands: [c] check state, [o] dump config, [h] help, [p] pause, [r] run, [s] "
                    "stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device"
                 << endl;
            return EXIT_SUCCESS;
        }

        bpo::notify(vm);

        CIntercomService service;
        CCustomCmd ddsCustomCmd(service);

        service.subscribeOnError([](const EErrorCode errorCode, const string& errorMsg) {
            cout << "DDS error received: error code: " << errorCode << ", error message: " << errorMsg << endl;
        });

        // subscribe to receive messages from DDS
        ddsCustomCmd.subscribe([&](const string& msg, const string& /*condition*/, uint64_t senderId) {
            vector<string> parts;
            boost::algorithm::split(parts, msg, boost::algorithm::is_any_of(":,"));
            if (parts[0] == "state-change") {
                {
                    unique_lock<mutex> lock(waitForStateMutex);
                    waitForStateMap[senderId] = parts[2];
                }
                waitForStateCV.notify_one();
            } else if (parts[0] == "state-changes-subscription") {
                // ok, stay silent
            } else if (parts[0] == "state-changes-unsubscription") {
                // ok, stay silent
            } else {
                cout << "Received: " << endl << msg << endl;
            }
        });

        service.start(sessionID);

        if (waitForState == "") {
            commandMode(command, topologyPath, ddsCustomCmd);
        } else {
            if (command != ' ') {
                commandMode(command, topologyPath, ddsCustomCmd);
            }
            waitMode(waitForState,
                     waitForStateMutex,
                     waitForStateCV,
                     waitForStateMap,
                     topologyPath,
                     ddsCustomCmd,
                     chrono::milliseconds(timeout));
        }
    } catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
