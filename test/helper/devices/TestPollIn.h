/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_POLLIN_H
#define FAIR_MQ_TEST_POLLIN_H

#include <FairMQDevice.h>
#include <FairMQLogger.h>
#include <fairmq/ProgOptions.h>
#include <thread>

namespace fair::mq::test
{

using namespace std;

class PollIn : public FairMQDevice
{
  public:
    PollIn()
        : fPollType(0)
    {}

  protected:
    auto InitTask() -> void override
    {
        fPollType = fConfig->GetProperty<int>("poll-type");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        vector<FairMQChannel*> chans;

        chans.push_back(&GetChannel("data1", 0));
        chans.push_back(&GetChannel("data2", 0));

        FairMQPollerPtr poller = nullptr;

        if (fPollType == 0)
        {
            poller = NewPoller(chans);
        }
        else if (fPollType == 1)
        {
            poller = NewPoller("data1", "data2");
        }
        else
        {
            LOG(error) << "wrong poll type provided: " << fPollType;
        }

        bool arrived1 = false;
        bool arrived2 = false;
        bool bothArrived = false;

        FairMQMessagePtr msg1(NewMessage());
        FairMQMessagePtr msg2(NewMessage());

        while (!bothArrived)
        {
            poller->Poll(100);

            if (fPollType == 0)
            {
                if (poller->CheckInput(0))
                {
                    LOG(debug) << "CheckInput(0) triggered";
                    if (Receive(msg1, "data1", 0) >= 0)
                    {
                        arrived1 = true;
                    }
                }

                if (poller->CheckInput(1))
                {
                    LOG(debug) << "CheckInput(1) triggered";
                    if (Receive(msg2, "data2", 0) >= 0)
                    {
                        arrived2 = true;
                    }
                }
            }
            else if (fPollType == 1)
            {
                if (poller->CheckInput("data1", 0))
                {
                    LOG(debug) << "CheckInput(\"data1\", 0) triggered";
                    if (Receive(msg1, "data1", 0) >= 0)
                    {
                        arrived1 = true;
                    }
                }

                if (poller->CheckInput("data2", 0))
                {
                    LOG(debug) << "CheckInput(\"data2\", 0) triggered";
                    if (Receive(msg2, "data2", 0) >= 0)
                    {
                        arrived2 = true;
                    }
                }
            }

            if (arrived1 && arrived2)
            {
                bothArrived = true;
                LOG(info) << "POLL test successfull";
            }
        }
    };

  private:
    int fPollType;
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_POLLIN_H */
