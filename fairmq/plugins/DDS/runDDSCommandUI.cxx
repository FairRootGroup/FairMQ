/********************************************************************************
 *  Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/sdk/commands/Commands.h>
#include <fairmq/States.h>

#include <DDS/dds_intercom.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <termios.h>   // raw mode console input
#include <thread>
#include <utility>
#include <unistd.h>

using namespace std;
using namespace dds::intercom_api;
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

struct StateSubscription
{
    const string& fTopologyPath;
    CCustomCmd& fDdsCustomCmd;

    explicit StateSubscription(const string& topologyPath, CCustomCmd& ddsCustomCmd)
        : fTopologyPath(topologyPath)
        , fDdsCustomCmd(ddsCustomCmd)
    {
        fDdsCustomCmd.send(Cmds(make<SubscribeToStateChange>()).Serialize(), fTopologyPath);
    }

    ~StateSubscription() {
        fDdsCustomCmd.send(Cmds(make<UnsubscribeFromStateChange>()).Serialize(), fTopologyPath);
        this_thread::sleep_for(chrono::milliseconds(100)); // give dds a chance to complete request
    }
};

void printControlsHelp()
{
    cout << "Use keys to control the devices:" << endl;
    cout << "[c] check states, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [k] complete init, [b] bind, [x] connect" << endl;
    cout << "To quit press Ctrl+C" << endl;
}

void sendCommand(const string& commandIn, const string& topologyPath, CCustomCmd& ddsCustomCmd) {
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
            ddsCustomCmd.send(Cmds(make<CheckState>()).Serialize(), topologyPath);
        } else if (command == "o") {
            cout << "> dumping config of the devices" << endl;
            ddsCustomCmd.send(Cmds(make<DumpConfig>()).Serialize(), topologyPath);
        } else if (command == "i") {
            cout << "> init devices" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::InitDevice)).Serialize(), topologyPath);
        } else if (command == "k") {
            cout << "> complete init" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::CompleteInit)).Serialize(), topologyPath);
        } else if (command == "b") {
            cout << "> bind devices" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::Bind)).Serialize(), topologyPath);
        } else if (command == "x") {
            cout << "> connect devices" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::Connect)).Serialize(), topologyPath);
        } else if (command == "j") {
            cout << "> init tasks" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::InitTask)).Serialize(), topologyPath);
        } else if (command == "r") {
            cout << "> run tasks" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::Run)).Serialize(), topologyPath);
        } else if (command == "s") {
            cout << "> stop devices" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::Stop)).Serialize(), topologyPath);
        } else if (command == "t") {
            cout << "> reset tasks" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::ResetTask)).Serialize(), topologyPath);
        } else if (command == "d") {
            cout << "> reset devices" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::ResetDevice)).Serialize(), topologyPath);
        } else if (command == "h") {
            cout << "> help" << endl;
            printControlsHelp();
        } else if (command == "q") {
            cout << "> end" << endl;
            ddsCustomCmd.send(Cmds(make<ChangeState>(fair::mq::Transition::End)).Serialize(), topologyPath);
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
        : fTransitionedCount(0)
    {
        if (targetState != "") {
            size_t n = targetState.find("->");
            if (n == string::npos) {
                fTargetStatePair.first = fair::mq::State::Ok;
                fTargetStatePair.second = fair::mq::GetState(targetState);
            } else {
                fTargetStatePair.first = fair::mq::GetState(targetState.substr(0, n));
                fTargetStatePair.second = fair::mq::GetState(targetState.substr(n + 2));
            }
        }
    }

    void Run(const chrono::milliseconds& timeout, const string& topologyPath, CCustomCmd& ddsCustomCmd, unsigned int numDevices, const string& command = "")
    {
        StateSubscription stateSubscription(topologyPath, ddsCustomCmd);

        if (command != "") {
            sendCommand(command, topologyPath, ddsCustomCmd);
        }

        // TODO once DDS provides an API to retrieve actual number of tasks, use it here
        auto condition = [&] {
            bool res = fTransitionedCount == numDevices;
            if (fTargetStatePair.first == fair::mq::State::Ok) {
                cout << "Waiting for " << numDevices << " devices to reach " << fTargetStatePair.second << ", condition check: " << res << endl;
            } else {
                cout << "Waiting for " << numDevices << " devices to reach " << fTargetStatePair.first << "->" << fTargetStatePair.second << ", condition check: " << res << endl;
            }
            return res;
        };

        unique_lock<mutex> lock(fMtx);

        if (timeout > chrono::milliseconds(0)) {
            if (!fCV.wait_for(lock, timeout, condition)) {
                throw runtime_error("timeout");
            }
        } else {
            fCV.wait(lock, condition);
        }

        // cout << "WaitMode.Run() finished" << endl;
    }

    void CountStates(fair::mq::State lastState, fair::mq::State currentState)
    {
        {
            unique_lock<mutex> lock(fMtx);
            if (fTargetStatePair.first == fair::mq::State::Ok) {
                if (fTargetStatePair.second == currentState) {
                    fTransitionedCount++;
                    // cout << "fTransitionedCount = " << fTransitionedCount << " for single value" << endl;
                }
            } else {
                if (fTargetStatePair.first == lastState && fTargetStatePair.second == currentState) {
                    fTransitionedCount++;
                    // cout << "fTransitionedCount = " << fTransitionedCount << " for double value" << endl;
                }
            }
        }
        fCV.notify_one();
    }

    mutex fMtx;
    condition_variable fCV;
    pair<fair::mq::State, fair::mq::State> fTargetStatePair;
    unsigned int fTransitionedCount;
};

int main(int argc, char* argv[])
{
    try {
        string sessionID;
        string command;
        string topologyPath;
        string targetState;
        unsigned int timeout;
        unsigned int numDevices(0);

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
            ("number-devices,n", bpo::value<unsigned int> (&numDevices)->default_value(0), "Number of devices (will be removed in the future)")
            ("help,h", "Produce help message");

        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(options).run(), vm);

        if (vm.count("help")) {
            cout << "FairMQ DDS Command UI" << endl << options << endl;
            cout << "Commands: [c] check state, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [k] complete init, [b] bind, [x] connect" << endl;
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
            Cmds cmds;
            cmds.Deserialize(msg);
            // cout << "Received " << cmds.Size() << " command(s) with total size of " << msg.length() << " bytes: " << endl;
            for (const auto& cmd : cmds) {
                // cout << " > " << cmd->GetType() << endl;
                switch (cmd->GetType()) {
                    case Type::state_change: {
                        cout << "Received state_change from " << static_cast<StateChange&>(*cmd).GetDeviceId() << ": " << static_cast<StateChange&>(*cmd).GetLastState() << "->" << static_cast<StateChange&>(*cmd).GetCurrentState() << endl;
                        if (static_cast<StateChange&>(*cmd).GetCurrentState() == fair::mq::State::Exiting) {
                            ddsCustomCmd.send(Cmds(make<StateChangeExitingReceived>()).Serialize(), to_string(senderId));
                        }
                        waitMode.CountStates(static_cast<StateChange&>(*cmd).GetLastState(), static_cast<StateChange&>(*cmd).GetCurrentState());
                    }
                    break;
                    case Type::state_change_subscription:
                        if (static_cast<StateChangeSubscription&>(*cmd).GetResult() != Result::Ok) {
                            cout << "State change subscription failed for " << static_cast<StateChangeSubscription&>(*cmd).GetDeviceId() << endl;
                        }
                    break;
                    case Type::state_change_unsubscription:
                        if (static_cast<StateChangeUnsubscription&>(*cmd).GetResult() != Result::Ok) {
                            cout << "State change unsubscription failed for " << static_cast<StateChangeUnsubscription&>(*cmd).GetDeviceId() << endl;
                        }
                    break;
                    case Type::transition_status: {
                        // if (static_cast<TransitionStatus&>(*cmd).GetResult() == Result::Ok) {
                        //     cout << "Device " << static_cast<TransitionStatus&>(*cmd).GetDeviceId() << " started to transition with " << static_cast<TransitionStatus&>(*cmd).GetTransition() << endl;
                        // } else {
                        //     cout << "Device " << static_cast<TransitionStatus&>(*cmd).GetDeviceId() << " cannot transition with " << static_cast<TransitionStatus&>(*cmd).GetTransition() << endl;
                        // }
                    }
                    break;
                    default:
                        cout << "Unexpected/unknown command received: " << cmd->GetType() << endl;
                        cout << "Origin: " << senderId << endl;
                    break;
                }
            }
        });

        service.start(sessionID);

        if (targetState == "") {
            sendCommand(command, topologyPath, ddsCustomCmd);
        } else {
            waitMode.Run(chrono::milliseconds(timeout), topologyPath, ddsCustomCmd, numDevices, command);
        }

        ddsCustomCmd.unsubscribe();
    } catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
