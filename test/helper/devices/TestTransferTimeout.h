/********************************************************************************
 * Copyright (C) 2015-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_TRANSFERTIMEOUT_H
#define FAIR_MQ_TEST_TRANSFERTIMEOUT_H

#include "TestDevice.h"

#include <string>

namespace fair::mq::test
{

/// Sends and receives on channels without a peer. Each transfer must end
/// with TransferCode::timeout, for a timeout of 200 ms and of 0 ms.
class TransferTimeout : public TestDevice
{
  protected:
    auto Run() -> void override
    {
        auto const timeout = static_cast<int64_t>(TransferCode::timeout);

        for (int ms : {200, 0}) {
            auto const suffix = " (" + std::to_string(ms) + " ms)";

            auto msgOut(NewMessage());
            auto msgIn(NewMessage());
            Check(Send(msgOut, "data-out", 0, ms) == timeout, "send msg times out" + suffix);
            Check(Receive(msgIn, "data-in", 0, ms) == timeout, "receive msg times out" + suffix);

            for (int numParts : {1, 2}) {
                auto const what = std::to_string(numParts) + " part(s) times out" + suffix;
                Parts partsOut;
                for (int i = 0; i < numParts; ++i) {
                    partsOut.AddPart(NewMessage(10));
                }
                Parts partsIn;
                Check(Send(partsOut, "data-out", 0, ms) == timeout, "send " + what);
                Check(Receive(partsIn, "data-in", 0, ms) == timeout, "receive " + what);
            }
        }

        Succeed();
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_TRANSFERTIMEOUT_H */
