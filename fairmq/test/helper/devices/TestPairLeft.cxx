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
        auto msg2{NewMessageFor("data", 0)};
        auto msg3{NewMessageFor("data", 0)};
        if (Send(msg1, "data") >= 0) counter++;
        if (Receive(msg2, "data") >= 0) counter++;
        if (Send(msg2, "data") >= 0) counter++;
        if (Receive(msg3, "data") >= 0) counter++;
        if (counter == 4) LOG(info) << "Simple empty message ping pong successfull";

        // Simple message with short text data
        auto msg4{NewSimpleMessageFor("data", 0, "testdata1234")};
        if (Send(msg4, "data") >= 0) counter++;
        if (counter == 5) LOG(info) << "Simple message with short text data successfull";

        assert(counter == 5);
    };
};

} // namespace test
} // namespace mq
} // namespace fair
