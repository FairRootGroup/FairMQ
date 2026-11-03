/********************************************************************************
 * Copyright (C) 2018-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>

#include <fairmq/Device.h>

#include <gtest/gtest.h>

#include <string>
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq;
using namespace fair::mq::test;

class BadDevice : public Device
{
  public:
    BadDevice()
    {
        fDeviceThread = thread([&](){
            EXPECT_THROW(RunStateMachine(), MessageError);
        });

        GetConfig()->SetProperty<size_t>("shm-segment-size", 100000000);
        SetTransport("shmem");

        ChangeStateOrThrow(Transition::InitDevice);
        WaitForState(State::InitializingDevice);
        ChangeStateOrThrow(Transition::CompleteInit);
        WaitForState(State::Initialized);

        parts.AddPart(NewMessage());
    }

    BadDevice(const BadDevice&) = delete;
    BadDevice(BadDevice&&) = delete;
    BadDevice& operator=(const BadDevice&) = delete;
    BadDevice& operator=(BadDevice&&) = delete;

    ~BadDevice() override
    {
        ChangeStateOrThrow(Transition::ResetDevice);

        if (fDeviceThread.joinable()) {
            fDeviceThread.join();
        }
    }

  private:
    thread fDeviceThread;
    Parts parts;
};

void RunErrorStateIn(string const& state, string const& control, string const& input = "")
{
    RunOptions options;
    options.control = control;
    options.input = input;
    auto const results = RunDevices({{"error_state_" + state + "_"}}, options);

    EXPECT_TRUE(ExitedWith(results.at(0), 1, "going to change to Error state from " + state + "()"));
}

TEST(ErrorState, static_InInit)
{
    RunErrorStateIn("Init", "static");
}

TEST(ErrorState, static_InBind)
{
    RunErrorStateIn("Bind", "static");
}

TEST(ErrorState, static_InConnect)
{
    RunErrorStateIn("Connect", "static");
}

TEST(ErrorState, static_InInitTask)
{
    RunErrorStateIn("InitTask", "static");
}

TEST(ErrorState, static_InPreRun)
{
    RunErrorStateIn("PreRun", "static");
}

TEST(ErrorState, static_InRun)
{
    RunErrorStateIn("Run", "static");
}

TEST(ErrorState, static_InPostRun)
{
    RunErrorStateIn("PostRun", "static");
}

TEST(ErrorState, static_InResetTask)
{
    RunErrorStateIn("ResetTask", "static");
}

TEST(ErrorState, static_InReset)
{
    RunErrorStateIn("Reset", "static");
}

TEST(ErrorState, interactive_InInit)
{
    RunErrorStateIn("Init", "interactive");
}

TEST(ErrorState, interactive_InBind)
{
    RunErrorStateIn("Bind", "interactive");
}

TEST(ErrorState, interactive_InConnect)
{
    RunErrorStateIn("Connect", "interactive");
}

TEST(ErrorState, interactive_InInitTask)
{
    RunErrorStateIn("InitTask", "interactive");
}

TEST(ErrorState, interactive_InPreRun)
{
    RunErrorStateIn("PreRun", "interactive");
}

TEST(ErrorState, interactive_InRun)
{
    RunErrorStateIn("Run", "interactive");
}

TEST(ErrorState, interactive_InPostRun)
{
    RunErrorStateIn("PostRun", "interactive");
}

TEST(ErrorState, interactive_InResetTask)
{
    RunErrorStateIn("ResetTask", "interactive", "q");
}

TEST(ErrorState, interactive_InReset)
{
    RunErrorStateIn("Reset", "interactive", "q");
}

#ifdef FAIRMQ_DEBUG_MODE
TEST(ErrorState, OrphanMessages)
{
    BadDevice badDevice;
}
#endif

} // namespace
