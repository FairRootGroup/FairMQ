/**
 * FairMQBuffer.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQBuffer.h"
#include "FairMQLogger.h"

FairMQBuffer::FairMQBuffer()
{
}

void FairMQBuffer::Run()
{
    LOG(INFO) << ">>>>>>> Run <<<<<<<";

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    bool received = false;
    while (fState == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        received = fPayloadInputs->at(0)->Receive(msg);

        if (received)
        {
            fPayloadOutputs->at(0)->Send(msg);
            received = false;
        }

        delete msg;
    }

    rateLogger.interrupt();
    rateLogger.join();
}

FairMQBuffer::~FairMQBuffer()
{
}
