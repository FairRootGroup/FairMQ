/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQBinSink.cxx
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQBinSink.h"
#include "FairMQLogger.h"

FairMQBinSink::FairMQBinSink()
{
}

void FairMQBinSink::Run()
{
    const FairMQChannel& dataInChannel = fChannels.at("data-in").at(0);

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        dataInChannel.Receive(msg);

        int inputSize = msg->GetSize();
        // int numInput = inputSize / sizeof(Content);
        // Content* input = reinterpret_cast<Content*>(msg->GetData());

        // for (int i = 0; i < numInput; ++i) {
        //     LOG(INFO) << (&input[i])->x << " " << (&input[i])->y << " " << (&input[i])->z << " " << (&input[i])->a << " " << (&input[i])->b;
        // }

        delete msg;
    }
}

FairMQBinSink::~FairMQBinSink()
{
}
