/**
 * FairMQSocket.cxx
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQSocket.h"
#include <sstream>
#include "FairMQLogger.h"


FairMQSocket::FairMQSocket(FairMQContext* context, int type, int num) :
  fBytesTx(0),
  fBytesRx(0),
  fMessagesTx(0),
  fMessagesRx(0)
{
  std::stringstream id;
  id << GetTypeString(type) << "." << num;
  fId = id.str();

  fSocket = zmq_socket(context->GetContext(), type);
  int rc = zmq_setsockopt(fSocket, ZMQ_IDENTITY, &fId, fId.Length());
  if (rc != 0) {
    std::stringstream logmsg;
    logmsg << "failed setting socket option, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }

  if (type == ZMQ_SUB) {
    rc = zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, NULL, 0);
    if (rc != 0) {
      std::stringstream logmsg2;
      logmsg2 << "failed setting socket option, reason: " << zmq_strerror(errno);
      FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
    }
  }

  std::stringstream logmsg3;
  logmsg3 << "created socket #" << fId;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg3.str());
}

FairMQSocket::~FairMQSocket()
{
}

TString FairMQSocket::GetId()
{
  return fId;
}

TString FairMQSocket::GetTypeString(int type)
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

void FairMQSocket::Bind(TString address)
{
  std::stringstream logmsg;
  logmsg << "bind socket #" << fId << " on " << address;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int rc = zmq_bind (fSocket, address);
  if (rc != 0) {
    std::stringstream logmsg2;
    logmsg2 << "failed binding socket #" << fId << ", reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
  }
}

void FairMQSocket::Connect(TString address)
{
  std::stringstream logmsg;
  logmsg << "connect socket #" << fId << " on " << address;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int rc = zmq_connect (fSocket, address);
  if (rc != 0) {
    std::stringstream logmsg2;
    logmsg2 << "failed connecting socket #" << fId << ", reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
  }
}

size_t FairMQSocket::Send(FairMQMessage* msg)
{
  int nbytes = zmq_msg_send (msg->GetMessage(), fSocket, 0);
  if (nbytes >= 0){
    fBytesTx += nbytes;
    ++fMessagesTx;
    return nbytes;
  }
  if (zmq_errno() == EAGAIN){
    return false;
  }
  std::stringstream logmsg;
  logmsg << "failed sending on socket #" << fId << ", reason: " << zmq_strerror(errno);
  FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  return nbytes;
}

size_t FairMQSocket::Receive(FairMQMessage* msg)
{
  int nbytes = zmq_msg_recv (msg->GetMessage(), fSocket, 0);
  if (nbytes >= 0){
    fBytesRx += nbytes;
    ++fMessagesRx;
    return nbytes;
  }
  if (zmq_errno() == EAGAIN){
    return false;
  }
  std::stringstream logmsg;
  logmsg << "failed receiving on socket #" << fId << ", reason: " << zmq_strerror(errno);
  FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  return nbytes;
}

void FairMQSocket::SetOption(int option, const void* value, size_t valueSize)
{
  int rc = zmq_setsockopt(fSocket, option, value, valueSize);
  if (rc < 0) {
    std::stringstream logmsg;
    logmsg << "failed setting socket option, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

void FairMQSocket::Close()
{
  if (fSocket == NULL){
    return;
  }

  int rc = zmq_close (fSocket);
  if (rc != 0) {
    std::stringstream logmsg;
    logmsg << "failed closing socket, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }

  fSocket = NULL;
}

void* FairMQSocket::GetSocket()
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
