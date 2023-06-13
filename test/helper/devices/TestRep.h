/********************************************************************************
 * Copyright (C) 2015-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_REP_H
#define FAIR_MQ_TEST_REP_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>
#include <thread>

namespace fair::mq::test
{

class Rep : public Device
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    bool check(signed long ret, const fair::mq::MessagePtr& msg)
    {
        auto content = std::string{static_cast<char*>(msg->GetData()), msg->GetSize()};
        LOG(info) << "Transferred " << static_cast<std::size_t>(ret) << " bytes, msg size: " << msg->GetSize() << ", content: " << content;
        return msg->GetSize() == static_cast<std::size_t>(ret) && content == "request";
    }

    auto Run() -> void override
    {
        int counter = 0;
        auto req1 = NewMessage();
        auto ret1 = Receive(req1, "data");
        if (ret1 >= 0) {
            LOG(info) << "Received request 1";
            if (check(ret1, req1)) {
                ++counter;
            }
            auto reply = NewSimpleMessageFor("data", 0, "reply");
            Send(reply, "data");
        }
        auto req2 = NewMessage();
        auto ret2 = Receive(req2, "data");
        if (ret2 >= 0) {
            LOG(info) << "Received request 2";
            if (check(ret2, req2)) {
                ++counter;
            }
            auto reply = NewSimpleMessageFor("data", 0, "reply");
            Send(reply, "data");
        }

        if (counter == 2) {
            LOG(info) << "REQ-REP test successfull";
        }
    };
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_REP_H */
