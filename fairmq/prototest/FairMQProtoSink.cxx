/**
 * FairMQProtoSink.cxx
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQProtoSink.h"
#include "FairMQLogger.h"

#include "payload.pb.h"

FairMQProtoSink::FairMQProtoSink()
{
}

void FairMQProtoSink::Run()
{
  LOG(INFO) << ">>>>>>> Run <<<<<<<";

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

  while ( fState == RUNNING ) {
    FairMQMessage* msg = fTransportFactory->CreateMessage();

    fPayloadInputs->at(0)->Receive(msg);

    sampler::Payload p;

    p.ParseFromArray(msg->GetData(), msg->GetSize());

    // for (int i = 0; i < p.data_size(); ++i) {
    //   const sampler::Payload::Content& content = p.data(i);
    //   LOG(INFO) << content.x() << " " << content.y() << " " << content.z() << " " << content.a() << " " << content.b();
    // }

    delete msg;
  }

  rateLogger.interrupt();
  rateLogger.join();
}

FairMQProtoSink::~FairMQProtoSink()
{
}
