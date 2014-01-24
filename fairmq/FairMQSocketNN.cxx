/**
 * FairMQSocketNN.cxx
 *
 * @since 2012-12-05
 * @author A. Rybalchenko
 */

#include <sstream>

#include "FairMQSocketNN.h"
#include "FairMQLogger.h"

FairMQSocketNN::FairMQSocketNN(const string& type, int num) :
  fBytesTx(0),
  fBytesRx(0),
  fMessagesTx(0),
  fMessagesRx(0)
{
  stringstream id;
  id << type << "." << num;
  fId = id.str();

  fSocket = nn_socket (AF_SP, GetConstant(type));
  if (type == "sub") {
    nn_setsockopt(fSocket, NN_SUB, NN_SUB_SUBSCRIBE, NULL, 0);
  }

  stringstream logmsg;
  logmsg << "created socket #" << fId;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());
}

string FairMQSocketNN::GetId()
{
  return fId;
}

void FairMQSocketNN::Bind(const string& address)
{
  stringstream logmsg;
  logmsg << "bind socket #" << fId << " on " << address;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int eid = nn_bind(fSocket, address.c_str());
  if (eid < 0) {
    stringstream logmsg2;
    logmsg2 << "failed binding socket #" << fId << ", reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
  }
}

void FairMQSocketNN::Connect(const string& address)
{
  stringstream logmsg;
  logmsg << "connect socket #" << fId << " to " << address;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int eid = nn_connect(fSocket, address.c_str());
  if (eid < 0) {
    stringstream logmsg2;
    logmsg2 << "failed connecting socket #" << fId << ", reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
  }
}

size_t FairMQSocketNN::Send(FairMQMessage* msg)
{
  void* ptr = msg->GetMessage();
  int rc = nn_send(fSocket, &ptr, NN_MSG, 0);
  if (rc < 0) {
    stringstream logmsg;
    logmsg << "failed sending on socket #" << fId << ", reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  } else {
    fBytesTx += rc;
    ++fMessagesTx;
  }

  return rc;
}

size_t FairMQSocketNN::Receive(FairMQMessage* msg)
{
  void* ptr = msg->GetMessage();
  int rc = nn_recv(fSocket, &ptr, NN_MSG, 0);
  if (rc < 0) {
    stringstream logmsg;
    logmsg << "failed receiving on socket #" << fId << ", reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  } else {
    fBytesRx += rc;
    ++fMessagesRx;
    msg->SetMessage(ptr, rc);
  }

  return rc;
}

void* FairMQSocketNN::GetSocket()
{
  return NULL;// dummy method to compy with the interface. functionality not possible in zeromq.
}

int FairMQSocketNN::GetSocket(int nothing)
{
  return fSocket;
}

void FairMQSocketNN::Close()
{
  nn_close(fSocket);
}

void FairMQSocketNN::SetOption(const string& option, const void* value, size_t valueSize)
{
  int rc = nn_setsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
  if (rc < 0) {
    stringstream logmsg;
    logmsg << "failed setting socket option, reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

unsigned long FairMQSocketNN::GetBytesTx()
{
  return fBytesTx;
}

unsigned long FairMQSocketNN::GetBytesRx()
{
  return fBytesRx;
}

unsigned long FairMQSocketNN::GetMessagesTx()
{
  return fMessagesTx;
}

unsigned long FairMQSocketNN::GetMessagesRx()
{
  return fMessagesRx;
}

int FairMQSocketNN::GetConstant(const string& constant)
{
  if (constant == "sub") return NN_SUB;
  if (constant == "pub") return NN_PUB;
  if (constant == "xsub") return NN_SUB; // TODO: is there XPUB, XSUB for nanomsg?
  if (constant == "xpub") return NN_PUB;
  if (constant == "push") return NN_PUSH;
  if (constant == "pull") return NN_PULL;
  if (constant == "snd-hwm") return NN_SNDBUF;
  if (constant == "rcv-hwm") return NN_RCVBUF;

  return -1;
}

FairMQSocketNN::~FairMQSocketNN()
{
  Close();
}
