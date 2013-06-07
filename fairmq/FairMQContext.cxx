/*
 * FairMQContext.cxx
 *
 *  Created on: Dec 5, 2012
 *      Author: dklein
 */

#include "FairMQContext.h"
#include <sstream>


const TString FairMQContext::PAYLOAD = "payload";
const TString FairMQContext::LOG = "log";
const TString FairMQContext::CONFIG = "config";
const TString FairMQContext::CONTROL = "control";

FairMQContext::FairMQContext(TString deviceId, TString contextId, Int_t numIoThreads)
{
  std::stringstream id;
  id << deviceId << "." << contextId;
  fId = id.str();

  fContext = new zmq::context_t(numIoThreads);
}

FairMQContext::~FairMQContext()
{
}

TString FairMQContext::GetId()
{
  return fId;
}

zmq::context_t* FairMQContext::GetContext()
{
  return fContext;
}

void FairMQContext::Close()
{
  fContext->close();
}

