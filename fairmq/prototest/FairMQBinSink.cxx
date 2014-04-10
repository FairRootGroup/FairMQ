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
    LOG(INFO) << ">>>>>>> Run <<<<<<<";

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    while (fState == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        fPayloadInputs->at(0)->Receive(msg);

        int inputSize = msg->GetSize();
        int numInput = inputSize / sizeof(Content);
        Content* input = reinterpret_cast<Content*>(msg->GetData());

        // for (int i = 0; i < numInput; ++i) {
        //     LOG(INFO) << (&input[i])->x << " " << (&input[i])->y << " " << (&input[i])->z << " " << (&input[i])->a << " " << (&input[i])->b;
        // }

        delete msg;
    }

    rateLogger.interrupt();
    rateLogger.join();
}

FairMQBinSink::~FairMQBinSink()
{
}
