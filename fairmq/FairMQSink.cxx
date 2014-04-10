/**
 * FairMQSink.cxx
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQSink.h"
#include "FairMQLogger.h"

FairMQSink::FairMQSink()
{
}

void FairMQSink::Run()
{
    LOG(INFO) << ">>>>>>> Run <<<<<<<";

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    while (fState == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        fPayloadInputs->at(0)->Receive(msg);

        delete msg;
    }

    rateLogger.interrupt();
    rateLogger.join();
}

FairMQSink::~FairMQSink()
{
}
