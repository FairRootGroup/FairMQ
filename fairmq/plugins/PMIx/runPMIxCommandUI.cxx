/********************************************************************************
 *  Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/sdk/commands/Commands.h>
#include <fairmq/States.h>

#include <fairlogger/Logger.h>

#include "PMIx.hpp"
#include "PMIxCommands.h"

#include <boost/program_options.hpp>

#include <condition_variable>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <termios.h> // raw mode console input
#include <thread>
#include <utility>
#include <unistd.h>
#include <vector>

using namespace std;
using namespace fair::mq::sdk::cmd;
namespace bpo = boost::program_options;

const std::map<fair::mq::Transition, fair::mq::State> expected =
{
    { fair::mq::Transition::InitDevice,   fair::mq::State::InitializingDevice },
    { fair::mq::Transition::CompleteInit, fair::mq::State::Initialized },
    { fair::mq::Transition::Bind,         fair::mq::State::Bound },
    { fair::mq::Transition::Connect,      fair::mq::State::DeviceReady },
    { fair::mq::Transition::InitTask,     fair::mq::State::Ready },
    { fair::mq::Transition::Run,          fair::mq::State::Running },
    { fair::mq::Transition::Stop,         fair::mq::State::Ready },
    { fair::mq::Transition::ResetTask,    fair::mq::State::DeviceReady },
    { fair::mq::Transition::ResetDevice,  fair::mq::State::Idle },
    { fair::mq::Transition::End,          fair::mq::State::Exiting }
};

struct StateSubscription
{
    pmix::Commands& fCommands;

    explicit StateSubscription(pmix::Commands& commands)
        : fCommands(commands)
    {
        fCommands.Send(Cmds(make<SubscribeToStateChange>(600000)).Serialize(Format::JSON));
    }

    ~StateSubscription()
    {
        fCommands.Send(Cmds(make<UnsubscribeFromStateChange>()).Serialize(Format::JSON));
        this_thread::sleep_for(chrono::milliseconds(100)); // give PMIx a chance to complete request
    }
};

struct MiniTopo
{
    explicit MiniTopo(unsigned int n)
        : fState(n, fair::mq::State::Ok)
    {}

    void WaitFor(const fair::mq::State state)
    {
        std::unique_lock<std::mutex> lk(fMtx);

        fCV.wait(lk, [&](){
            unsigned int count = std::count_if(fState.cbegin(), fState.cend(), [=](const auto& s) {
                return s == state;
            });

            bool result = count == fState.size();
            cout << "expecting " << state << " for " << fState.size() << " devices, found " << count << ", condition: " << result << endl;
            return result;
        });
    }

    void Update(uint32_t rank, const fair::mq::State state)
    {
        try {
            {
                std::lock_guard<std::mutex> lk(fMtx);
                fState.at(rank - 1) = state;
            }
            fCV.notify_one();
        } catch (const std::exception& e) {
            LOG(error) << "Exception in Update: " << e.what();
        }
    }

  private:
    vector<fair::mq::State> fState;
    std::mutex fMtx;
    std::condition_variable fCV;
};

int main(int argc, char* argv[])
{
    try {
        unsigned int numDevices;

        bpo::options_description options("Common options");
        options.add_options()
            ("number-devices,n", bpo::value<unsigned int>(&numDevices)->default_value(0), "Number of devices (will be removed in the future)")
            ("help,h", "Produce help message");

        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(options).run(), vm);

        if (vm.count("help")) {
            cout << "FairMQ DDS Command UI" << endl << options << endl;
            cout << "Commands: [c] check state, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [k] complete init, [b] bind, [x] connect" << endl;
            return EXIT_SUCCESS;
        }

        bpo::notify(vm);

        fair::Logger::SetConsoleSeverity(fair::Severity::debug);
        fair::Logger::SetConsoleColor(true);
        fair::Logger::SetVerbosity(fair::Verbosity::low);

        pmix::proc process;

        if (!pmix::initialized()) {
            process = pmix::init();
            LOG(warn) << "pmix::init() OK: " << process << ", version=" << pmix::get_version();
        }

        pmix::proc all(process);
        all.rank = pmix::rank::wildcard;
        pmix::fence({all});
        LOG(warn) << "pmix::fence() [pmix::init] OK";

        MiniTopo topo(numDevices);
        pmix::Commands commands(process);

        commands.Subscribe([&](const string& msg, const pmix::proc& sender) {
            // LOG(info) << "Received '" << msg << "' from " << sender;
            Cmds cmds;
            cmds.Deserialize(msg, Format::JSON);
            // cout << "Received " << cmds.Size() << " command(s) with total size of " << msg.length() << " bytes: " << endl;
            for (const auto& cmd : cmds) {
                // cout << " > " << cmd->GetType() << endl;
                switch (cmd->GetType()) {
                    case Type::state_change: {
                        cout << "Received state_change from " << static_cast<StateChange&>(*cmd).GetDeviceId() << ": " << static_cast<StateChange&>(*cmd).GetLastState() << "->" << static_cast<StateChange&>(*cmd).GetCurrentState() << endl;
                        topo.Update(sender.rank, static_cast<StateChange&>(*cmd).GetCurrentState());
                        if (static_cast<StateChange&>(*cmd).GetCurrentState() == fair::mq::State::Exiting) {
                            commands.Send(Cmds(make<StateChangeExitingReceived>()).Serialize(Format::JSON), {sender});
                        }
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
                        if (static_cast<TransitionStatus&>(*cmd).GetResult() == Result::Ok) {
                            cout << "Device " << static_cast<TransitionStatus&>(*cmd).GetDeviceId() << " started to transition with " << static_cast<TransitionStatus&>(*cmd).GetTransition() << endl;
                        } else {
                            cout << "Device " << static_cast<TransitionStatus&>(*cmd).GetDeviceId() << " cannot transition with " << static_cast<TransitionStatus&>(*cmd).GetTransition() << endl;
                        }
                    }
                    break;
                    case Type::current_state:
                        cout << "Device " << static_cast<CurrentState&>(*cmd).GetDeviceId() << " is in " << static_cast<CurrentState&>(*cmd).GetCurrentState() << " state" << endl;
                    break;
                    case Type::config:
                        cout << "Received config for device " << static_cast<Config&>(*cmd).GetDeviceId() << ":\n" << static_cast<Config&>(*cmd).GetConfig() << endl;
                    break;
                    default:
                        cout << "Unexpected/unknown command received: " << cmd->GetType() << endl;
                        cout << "Origin: " << sender << endl;
                    break;
                }
            }
        });

        pmix::fence({all});
        LOG(warn) << "pmix::fence() [subscribed] OK";

        StateSubscription stateSubscription(commands);

        for (auto transition : { fair::mq::Transition::InitDevice,
                                 fair::mq::Transition::CompleteInit,
                                 fair::mq::Transition::Bind,
                                 fair::mq::Transition::Connect,
                                 fair::mq::Transition::InitTask,
                                 fair::mq::Transition::Run,
                                 fair::mq::Transition::Stop,
                                 fair::mq::Transition::ResetTask,
                                 fair::mq::Transition::ResetDevice,
                                 fair::mq::Transition::End }) {
            commands.Send(Cmds(make<ChangeState>(transition)).Serialize(Format::JSON));
            topo.WaitFor(expected.at(transition));
        }
    } catch (exception& e) {
        LOG(error) << "Error: " << e.what();
        return EXIT_FAILURE;
    }
    LOG(warn) << "exiting";
    return EXIT_SUCCESS;
}
