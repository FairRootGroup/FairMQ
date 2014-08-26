/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMerger.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQMerger.h"
#include "FairMQPoller.h"

FairMQMerger::FairMQMerger()
{
}

FairMQMerger::~FairMQMerger()
{
}

void FairMQMerger::Run()
{
    LOG(INFO) << ">>>>>>> Run <<<<<<<";

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    FairMQPoller* poller = fTransportFactory->CreatePoller(*fPayloadInputs);

    int received = 0;

    while (fState == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        poller->Poll(100);

        for (int i = 0; i < fNumInputs; i++)
        {
            if (poller->CheckInput(i))
            {
                received = fPayloadInputs->at(i)->Receive(msg);
            }
            if (received > 0)
            {
                fPayloadOutputs->at(0)->Send(msg);
                received = 0;
            }
        }

        delete msg;
    }

    delete poller;

    try {
        rateLogger.interrupt();
        rateLogger.join();
    } catch(boost::thread_resource_error& e) {
        LOG(ERROR) << e.what();
    }

    FairMQDevice::Shutdown();

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fRunningMutex);
    fRunningFinished = true;
    fRunningCondition.notify_one();
}
