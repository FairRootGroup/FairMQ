/********************************************************************************
 * Copyright (C) 2018-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>
#include <helper/StateParam.h>

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

using ErrorState = StateTest;

TEST_P(ErrorState, ChangeState)
{
    auto const& state = GetParam().state;
    RunOptions options;
    options.control = GetParam().control;
    options.input = GetParam().Input();
    auto const results = RunDevices({{"error_state_" + state + "_"}}, options);

    EXPECT_TRUE(ExitedWith(results.at(0), 1, "going to change to Error state from " + state + "()"));
}

INSTANTIATE_TEST_SUITE_P(, ErrorState, WithControls(HookStates()), StateParamName());

#ifdef FAIRMQ_DEBUG_MODE
TEST(OrphanMessages, shmem)
{
    BadDevice badDevice;
}
#endif

} // namespace
