/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_POLLIN_H
#define FAIR_MQ_TEST_POLLIN_H

#include "TestDevice.h"

#include <fairmq/ProgOptions.h>

#include <vector>

namespace fair::mq::test
{

/// Polls two channels until a message arrived on each.
/// poll-type 0: poll a vector of sub-channels; 1: poll by channel names.
class PollIn : public TestDevice
{
  protected:
    auto InitTask() -> void override
    {
        fPollType = fConfig->GetProperty<int>("poll-type");
        Check(fPollType == 0 || fPollType == 1, "poll-type is 0 or 1");
    }

    auto Run() -> void override
    {
        PollerPtr poller;
        if (fPollType == 0) {
            std::vector<Channel*> chans{&GetChannel("data1", 0), &GetChannel("data2", 0)};
            poller = NewPoller(chans);
        } else {
            poller = NewPoller("data1", "data2");
        }

        auto checkInput = [&](int index, std::string const& name) {
            return fPollType == 0 ? poller->CheckInput(index) : poller->CheckInput(name, 0);
        };

        bool arrived1 = false;
        bool arrived2 = false;
        while (!arrived1 || !arrived2) {
            poller->Poll(100);
            if (checkInput(0, "data1")) {
                auto msg = NewMessage();
                arrived1 = arrived1 || Receive(msg, "data1", 0) >= 0;
            }
            if (checkInput(1, "data2")) {
                auto msg = NewMessage();
                arrived2 = arrived2 || Receive(msg, "data2", 0) >= 0;
            }
        }

        Succeed();
    }

  private:
    int fPollType = 0;
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_POLLIN_H */
