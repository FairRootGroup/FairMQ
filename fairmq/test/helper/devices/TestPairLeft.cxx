/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>

namespace fair
{
namespace mq
{
namespace test
{

class PairLeft : public FairMQDevice
{
  protected:
    auto Init() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Reset() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto Run() -> void override
    {
        int counter{0};

        // Simple empty message ping pong
        auto msg1{NewMessageFor("data", 0)};
        if (Send(msg1, "data") >= 0) counter++;
        auto msg2{NewMessageFor("data", 0)};
        if (Receive(msg2, "data") >= 0) counter++;
        auto msg3{NewMessageFor("data", 0)};
        if (Send(msg3, "data") >= 0) counter++;
        auto msg4{NewMessageFor("data", 0)};
        if (Receive(msg4, "data") >= 0) counter++;
        if (counter == 4) LOG(info) << "Simple empty message ping pong successfull";

        // Simple message with short text data
        auto msg5{NewSimpleMessageFor("data", 0, "testdata1234")};
        LOG(info) << "Will send msg5";
        if (Send(msg5, "data") >= 0) counter++;
        LOG(info) << "Sent msg5";
        if (counter == 5) LOG(info) << "Simple message with short text data successfull";

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        assert(counter == 5);
    };
};

} // namespace test
} // namespace mq
} // namespace fair
