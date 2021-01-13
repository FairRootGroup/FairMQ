/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PAIRLEFT_H
#define FAIR_MQ_TEST_PAIRLEFT_H

#include <FairMQDevice.h>

#include <cstddef>
#include <thread>

namespace fair::mq::test
{

class PairLeft : public FairMQDevice
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        int counter{0};

        // Simple empty message ping pong
        auto msg1(NewMessageFor("data", 0));
        if (Send(msg1, "data") >= 0) counter++;
        auto msg2(NewMessageFor("data", 0));
        if (Receive(msg2, "data") >= 0) counter++;
        auto msg3(NewMessageFor("data", 0));
        if (Send(msg3, "data") >= 0) counter++;
        auto msg4(NewMessageFor("data", 0));
        if (Receive(msg4, "data") >= 0) counter++;
        if (counter == 4) LOG(info) << "Simple empty message ping pong successfull";

        // Simple message with short text data
        auto msg5(NewSimpleMessageFor("data", 0, "testdata1234"));
        if (Send(msg5, "data") >= 0) counter++;
        auto msg6(NewMessageFor("data", 0));
        auto ret = Receive(msg6, "data");
        if (ret > 0) {
            auto content = std::string{static_cast<char*>(msg6->GetData()), msg6->GetSize()};
            LOG(info) << ret << ", " << msg6->GetSize() << ", '" << content << "'";
            if (msg6->GetSize() == static_cast<std::size_t>(ret) && content == "testdata1234") counter++;
        }
        if (counter == 6) LOG(info) << "Simple message with short text data successfull";

        assert(counter == 6);
        if (counter == 6) LOG(info) << "PAIR test successfull.";
    };
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PAIRLEFT_H */
