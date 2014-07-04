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

    size_t bytes_received = 0;

    while (fState == RUNNING)
    {
        bytes_received = fPayloadInputs->at(0)->Receive(msg);
        if (bytes_received)
        {
            fPayloadOutputs->at(0)->Send(msg);
            bytes_received = 0;
        }
    }

    delete msg;

    rateLogger.interrupt();
    rateLogger.join();
}
