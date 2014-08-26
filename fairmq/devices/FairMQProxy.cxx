/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProxy.cxx
 *
 * @since 2013-10-02
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQProxy.h"

FairMQProxy::FairMQProxy()
{
}

FairMQProxy::~FairMQProxy()
{
}

void FairMQProxy::Run()
{
    LOG(INFO) << ">>>>>>> Run <<<<<<<";

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    FairMQMessage* msg = fTransportFactory->CreateMessage();

    int received = 0;

    while (fState == RUNNING)
    {
        received = fPayloadInputs->at(0)->Receive(msg);
        if (received > 0)
        {
            fPayloadOutputs->at(0)->Send(msg);
            received = 0;
        }
    }

    delete msg;

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
