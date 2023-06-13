/********************************************************************************
 * Copyright (C) 2015-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_REQ_H
#define FAIR_MQ_TEST_REQ_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>
#include <thread>

namespace fair::mq::test
{

class Req : public Device
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto request = NewSimpleMessageFor("data", 0, "request");
        Send(request, "data");

        auto reply = NewMessage();
        if (Receive(reply, "data") >= 0) {
            LOG(info) << "received reply";
            auto content = std::string{static_cast<char*>(reply->GetData()), reply->GetSize()};
            LOG(info) << "Transferred reply of size: " << reply->GetSize() << ", content: " << content;
            if (content != "reply") {
                ChangeStateOrThrow(Transition::ErrorFound);
            }
        }
    };
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_REQ_H */
