/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PAIRRIGHT_H
#define FAIR_MQ_TEST_PAIRRIGHT_H

#include <FairMQDevice.h>
#include <cstddef>
#include <string>
#include <thread>

namespace fair::mq::test
{

class PairRight : public FairMQDevice
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
        if (Receive(msg1, "data") >= 0) counter++;
        auto msg2(NewMessageFor("data", 0));
        if (Send(msg2, "data") >= 0) counter++;
        auto msg3(NewMessageFor("data", 0));
        if (Receive(msg3, "data") >= 0) counter++;
        auto msg4(NewMessageFor("data", 0));
        if (Send(msg4, "data") >= 0) counter++;
        if (counter == 4) LOG(info) << "Simple empty message ping pong successfull";

        // Simple message with short text data
        auto msg5(NewMessageFor("data", 0));
        auto ret = Receive(msg5, "data");
        if (ret > 0) {
            auto content = std::string{static_cast<char*>(msg5->GetData()), msg5->GetSize()};
            LOG(info) << ret << ", " << msg5->GetSize() << ", '" << content << "'";
            if (msg5->GetSize() == static_cast<std::size_t>(ret) && content == "testdata1234") counter++;
        }
        auto msg6(NewSimpleMessageFor("data", 0, "testdata1234"));
        if (Send(msg6, "data") >= 0) counter++;
        if (counter == 6) LOG(info) << "Simple message with short text data successfull";
    };
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PAIRRIGHT_H */
