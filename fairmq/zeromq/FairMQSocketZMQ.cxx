/**
 * FairMQSocketZMQ.cxx
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#include <sstream>

#include "FairMQSocketZMQ.h"
#include "FairMQLogger.h"

boost::shared_ptr<FairMQContextZMQ> FairMQSocketZMQ::fContext = boost::shared_ptr<FairMQContextZMQ>(new FairMQContextZMQ(1)); // TODO: numIoThreads!

FairMQSocketZMQ::FairMQSocketZMQ(const string& type, int num) :
  fBytesTx(0),
  fBytesRx(0),
  fMessagesTx(0),
  fMessagesRx(0)
{
  stringstream id;
  id << type << "." << num;
  fId = id.str();

  fSocket = zmq_socket(fContext->GetContext(), GetConstant(type));

  int rc = zmq_setsockopt(fSocket, ZMQ_IDENTITY, &fId, fId.length());
  if (rc != 0) {
    LOG(ERROR) << "failed setting socket option, reason: " << zmq_strerror(errno);
  }

  if (type == "sub") {
    rc = zmq_setsockopt(fSocket, ZMQ_SUBSCRIBE, NULL, 0);
    if (rc != 0) {
      LOG(ERROR) << "failed setting socket option, reason: " << zmq_strerror(errno);
    }
  }

  LOG(INFO) << "created socket #" << fId;
}

string FairMQSocketZMQ::GetId()
{
  return fId;
}

void FairMQSocketZMQ::Bind(const string& address)
{
  LOG(INFO) << "bind socket #" << fId << " on " << address;

  int rc = zmq_bind (fSocket, address.c_str());
  if (rc != 0) {
    LOG(ERROR) << "failed binding socket #" << fId << ", reason: " << zmq_strerror(errno);
  }
}

void FairMQSocketZMQ::Connect(const string& address)
{
  LOG(INFO) << "connect socket #" << fId << " on " << address;

  int rc = zmq_connect (fSocket, address.c_str());
  if (rc != 0) {
    LOG(ERROR) << "failed connecting socket #" << fId << ", reason: " << zmq_strerror(errno);
  }
}

size_t FairMQSocketZMQ::Send(FairMQMessage* msg)
{
  int nbytes = zmq_msg_send (static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, 0);
  if (nbytes >= 0){
    fBytesTx += nbytes;
    ++fMessagesTx;
    return nbytes;
  }
  if (zmq_errno() == EAGAIN){
    return false;
  }
  LOG(ERROR) << "failed sending on socket #" << fId << ", reason: " << zmq_strerror(errno);
  return nbytes;
}

size_t FairMQSocketZMQ::Receive(FairMQMessage* msg)
{
  int nbytes = zmq_msg_recv (static_cast<zmq_msg_t*>(msg->GetMessage()), fSocket, 0);
  if (nbytes >= 0){
    fBytesRx += nbytes;
    ++fMessagesRx;
    return nbytes;
  }
  if (zmq_errno() == EAGAIN){
    return false;
  }
  LOG(ERROR) << "failed receiving on socket #" << fId << ", reason: " << zmq_strerror(errno);
  return nbytes;
}

void FairMQSocketZMQ::Close()
{
  if (fSocket == NULL){
    return;
  }

  int rc = zmq_close (fSocket);
  if (rc != 0) {
    LOG(ERROR) << "failed closing socket, reason: " << zmq_strerror(errno);
  }

  fSocket = NULL;
}

void* FairMQSocketZMQ::GetSocket()
{
  return fSocket;
}

int FairMQSocketZMQ::GetSocket(int nothing)
{
  // dummy method to comply with the interface. functionality not possible in zeromq.
  return -1;
}

void FairMQSocketZMQ::SetOption(const string& option, const void* value, size_t valueSize)
{
  int rc = zmq_setsockopt(fSocket, GetConstant(option), value, valueSize);
  if (rc < 0) {
    LOG(ERROR) << "failed setting socket option, reason: " << zmq_strerror(errno);
  }
}

unsigned long FairMQSocketZMQ::GetBytesTx()
{
  return fBytesTx;
}

unsigned long FairMQSocketZMQ::GetBytesRx()
{
  return fBytesRx;
}

unsigned long FairMQSocketZMQ::GetMessagesTx()
{
  return fMessagesTx;
}

unsigned long FairMQSocketZMQ::GetMessagesRx()
{
  return fMessagesRx;
}

int FairMQSocketZMQ::GetConstant(const string& constant)
{
  if (constant == "sub") return ZMQ_SUB;
  if (constant == "pub") return ZMQ_PUB;
  if (constant == "xsub") return ZMQ_XSUB;
  if (constant == "xpub") return ZMQ_XPUB;
  if (constant == "push") return ZMQ_PUSH;
  if (constant == "pull") return ZMQ_PULL;
  if (constant == "snd-hwm") return ZMQ_SNDHWM;
  if (constant == "rcv-hwm") return ZMQ_RCVHWM;

  return -1;
}

FairMQSocketZMQ::~FairMQSocketZMQ()
{
}
