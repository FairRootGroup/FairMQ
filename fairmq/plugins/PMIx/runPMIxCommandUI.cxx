/********************************************************************************
 *  Copyright (C) 2014-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

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
    } catch (exception& e) {
        LOG(error) << "Error: " << e.what();
        return EXIT_FAILURE;
    }
    LOG(warn) << "exiting";
    return EXIT_SUCCESS;
}
