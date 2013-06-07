/*
 * FairMQMessage.cxx
 *
 *  Created on: Dec 5, 2012
 *      Author: dklein
 */

#include "FairMQMessage.h"
#include "FairMQLogger.h"


FairMQMessage::FairMQMessage()
{
  try {
    fMessage = new zmq::message_t();
  } catch (zmq::error_t& e) {
    std::stringstream logmsg;
    logmsg << "failed allocating new message, reason: " << e.what();
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

FairMQMessage::FairMQMessage(void* data_, size_t size_, zmq::free_fn* ffn_, void* hint_/*= NULL*/)
{
  try {
    fMessage = new zmq::message_t(data_, size_, ffn_, hint_);
  } catch (zmq::error_t& e) {
    std::stringstream logmsg;
    logmsg << "failed allocating new message, reason: " << e.what();
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

FairMQMessage::~FairMQMessage()
{
  delete fMessage;
}

zmq::message_t* FairMQMessage::GetMessage()
{
  return fMessage;
}

Int_t FairMQMessage::Size()
{
  return fMessage->size();
}

Bool_t FairMQMessage::Copy(FairMQMessage* msg)
{
  Bool_t result = false;

  try {
    fMessage->copy(msg->GetMessage());
  } catch (zmq::error_t& e) {
    std::stringstream logmsg;
    logmsg << "failed copying message, reason: " << e.what();
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }

  return result;
}

