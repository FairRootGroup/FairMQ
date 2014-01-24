/**
 * FairMQMessageZMQ.cxx
 *
 * @since 2012-12-05
 * @author: D. Klein, A. Rybalchenko
 */

#include <cstdlib>

#include "FairMQMessageZMQ.h"
#include "FairMQLogger.h"


FairMQMessageZMQ::FairMQMessageZMQ()
{
  int rc = zmq_msg_init (&fMessage);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed initializing message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

FairMQMessageZMQ::FairMQMessageZMQ(size_t size)
{
  int rc = zmq_msg_init_size (&fMessage, size);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed initializing message with size, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

FairMQMessageZMQ::FairMQMessageZMQ(void* data, size_t size)
{
  int rc = zmq_msg_init_data (&fMessage, data, size, &CleanUp, NULL);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed initializing message with data, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

void FairMQMessageZMQ::Rebuild()
{
  int rc = zmq_msg_close (&fMessage);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed closing message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }

  rc = zmq_msg_init (&fMessage);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed initializing message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

void FairMQMessageZMQ::Rebuild(size_t size)
{
  int rc = zmq_msg_close (&fMessage);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed closing message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }

  rc = zmq_msg_init_size (&fMessage, size);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed initializing message with size, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

void FairMQMessageZMQ::Rebuild(void* data, size_t size)
{
  int rc = zmq_msg_close (&fMessage);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed closing message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }

  rc = zmq_msg_init_data (&fMessage, data, size, &CleanUp, NULL);
  if (rc != 0) {
    stringstream logmsg2;
    logmsg2 << "failed initializing message with data, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
  }
}

void* FairMQMessageZMQ::GetMessage()
{
  return &fMessage;
}

void* FairMQMessageZMQ::GetData()
{
  return zmq_msg_data (&fMessage);
}

size_t FairMQMessageZMQ::GetSize()
{
  return zmq_msg_size (&fMessage);
}

void FairMQMessageZMQ::SetMessage(void* data, size_t size)
{
  // dummy method to comply with the interface. functionality not allowed in zeromq.
}

void FairMQMessageZMQ::Copy(FairMQMessage* msg)
{
  int rc = zmq_msg_copy (&fMessage, &(static_cast<FairMQMessageZMQ*>(msg)->fMessage));
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed copying message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

void FairMQMessageZMQ::CleanUp(void* data, void* hint)
{
  free (data);
}

FairMQMessageZMQ::~FairMQMessageZMQ()
{
  int rc = zmq_msg_close (&fMessage);
  if (rc != 0) {
    stringstream logmsg;
    logmsg << "failed closing message with data, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}
