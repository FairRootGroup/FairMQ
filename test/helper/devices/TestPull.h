/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_PULL_H
#define FAIR_MQ_TEST_PULL_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>
#include <thread>

namespace fair::mq::test
{

using namespace std;

class Pull : public Device
{
  protected:
    auto InitTask() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        int counter = 0;

        auto msg1 = NewMessageFor("data", 0);

        if (Receive(msg1, "data") >= 0) {
            ++counter;
        }

        auto msg2 = NewMessageFor("data", 0);

        auto ret = Receive(msg2, "data");
        if (ret >= 0) {
            auto content = std::string{static_cast<char*>(msg2->GetData()), msg2->GetSize()};
            LOG(info) << "Transferred " << static_cast<std::size_t>(ret) << " bytes, msg size: " << msg2->GetSize() << ", content: " << content;
            if (msg2->GetSize() == static_cast<std::size_t>(ret) && content == "testdata1234") {
                ++counter;
            }
        }

        if (counter == 2) {
            LOG(info) << "PUSH-PULL test successfull";
        }
    };
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_PULL_H */
