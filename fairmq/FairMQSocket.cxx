/*
 * FairMQSocket.cxx
 *
 *  Created on: Dec 5, 2012
 *      Author: dklein
 */

#include "FairMQSocket.h"
#include <sstream>
#include "FairMQLogger.h"


const TString FairMQSocket::TCP = "tcp://";
const TString FairMQSocket::IPC = "ipc://";
const TString FairMQSocket::INPROC = "inproc://";

FairMQSocket::FairMQSocket(FairMQContext* context, int type, int num) :
  fBytesTx(0),
  fBytesRx(0),
  fMessagesTx(0),
  fMessagesRx(0)
{
  std::stringstream id;
  id << context->GetId() << "." << GetTypeString(type) << "." << num;
  fId = id.str();

  fSocket = new zmq::socket_t(*context->GetContext(), type);
  fSocket->setsockopt(ZMQ_IDENTITY, &fId, fId.Length());
  if (type == ZMQ_SUB) {
    fSocket->setsockopt(ZMQ_SUBSCRIBE, NULL, 0);
  }

  std::stringstream logmsg;
  logmsg << "created socket #" << fId;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());
}

FairMQSocket::~FairMQSocket()
{
}

TString FairMQSocket::GetId()
{
  return fId;
}

TString FairMQSocket::GetTypeString(Int_t type)
{
  switch (type) {
  case ZMQ_SUB:
    return "sub";
  case ZMQ_PUB:
    return "pub";
  case ZMQ_PUSH:
    return "push";
  case ZMQ_PULL:
    return "pull";
  default:
    return "";
  }
}

Bool_t FairMQSocket::Bind(TString address)
{
  Bool_t result = true;

  try {
    if ( address.Length() > 0 /*!address.empty()*/) {
      std::stringstream logmsg;
      logmsg << "bind socket #" << fId << " on " << address;
      FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());
      fSocket->bind(address.Data());
    }
  } catch (zmq::error_t& e) {
    std::stringstream logmsg2;
    logmsg2 << "failed binding socket #" << fId << ", reason: " << e.what();
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
    result = false;
  }

  return result;
}

Bool_t FairMQSocket::Connect(TString address)
{
  Bool_t result = true;

  try {
    if ( address.Length() > 0 /*!address.empty()*/) {
      std::stringstream logmsg;
      logmsg << "connect socket #" << fId << " to " << address;
      FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());
      fSocket->connect(address.Data());
    }
  } catch (zmq::error_t& e) {
    std::stringstream logmsg2;
    logmsg2 << "failed connecting socket #" << fId << ", reason: " << e.what();
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
    result = false;
  }

  return result;
}

Bool_t FairMQSocket::Send(FairMQMessage* msg)
{
  Bool_t result = false;

  try {
    fBytesTx += msg->Size();
    ++fMessagesTx;
    result = fSocket->send(*msg->GetMessage()); // use send(*msg->GetMessage(), ZMQ_DONTWAIT) for non-blocking call
  } catch (zmq::error_t& e) {
    std::stringstream logmsg;
    logmsg << "failed sending on socket #" << fId << ", reason: " << e.what();
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
    result = false;
  }

  return result;
}

Bool_t FairMQSocket::Receive(FairMQMessage* msg)
{
  Bool_t result = false;

  try {
    result = fSocket->recv(msg->GetMessage());
    fBytesRx += msg->Size();
    ++fMessagesRx;
  } catch (zmq::error_t& e) {
    std::stringstream logmsg;
    logmsg << "failed receiving on socket #" << fId << ", reason: " << e.what();
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
    result = false;
  }

  return result;
}

void FairMQSocket::Close()
{
  fSocket->close();
}

zmq::socket_t* FairMQSocket::GetSocket()
{
  return fSocket;
}

ULong_t FairMQSocket::GetBytesTx()
{
  return fBytesTx;
}

ULong_t FairMQSocket::GetBytesRx()
{
  return fBytesRx;
}

ULong_t FairMQSocket::GetMessagesTx()
{
  return fMessagesTx;
}

ULong_t FairMQSocket::GetMessagesRx()
{
  return fMessagesRx;
}
