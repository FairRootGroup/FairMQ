/**
 * FairMQMessageZMQ.cxx
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko, N. Winckler
 */

#include <cstdlib>

#include "FairMQMessageZMQ.h"
#include "FairMQLogger.h"


FairMQMessageZMQ::FairMQMessageZMQ() :
  fMessage(new zmq_msg_t())
{
  int rc = zmq_msg_init (fMessage);
  if (rc != 0) {
    LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
  }
}

FairMQMessageZMQ::FairMQMessageZMQ(size_t size) :
  fMessage(new zmq_msg_t())
{
  int rc = zmq_msg_init_size (fMessage, size);
  if (rc != 0) {
    LOG(ERROR) << "failed initializing message with size, reason: " << zmq_strerror(errno);
  }
}

FairMQMessageZMQ::FairMQMessageZMQ(void* data, size_t size) :
  fMessage(new zmq_msg_t())
{
  int rc = zmq_msg_init_data (fMessage, data, size, &CleanUp, NULL);
  if (rc != 0) {
    LOG(ERROR) << "failed initializing message with data, reason: " << zmq_strerror(errno);
  }
}

void FairMQMessageZMQ::Rebuild()
{
  CloseMessage();
  int rc = zmq_msg_init (fMessage);
  if (rc != 0) {
    LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
  }
}

void FairMQMessageZMQ::Rebuild(size_t size)
{
  CloseMessage();
  int rc = zmq_msg_init_size (fMessage, size);
  if (rc != 0) {
    LOG(ERROR) << "failed initializing message with size, reason: " << zmq_strerror(errno);
  }
}

void FairMQMessageZMQ::Rebuild(void* data, size_t size)
{
  CloseMessage();
  int rc = zmq_msg_init_data (fMessage, data, size, &CleanUp, NULL);
  if (rc != 0) {
    LOG(ERROR) << "failed initializing message with data, reason: " << zmq_strerror(errno);
  }
}

void* FairMQMessageZMQ::GetMessage()
{
  return fMessage;
}

void* FairMQMessageZMQ::GetData()
{
  return zmq_msg_data (fMessage);
}

size_t FairMQMessageZMQ::GetSize()
{
  return zmq_msg_size (fMessage);
}

void FairMQMessageZMQ::SetMessage(void* data, size_t size)
{
  // dummy method to comply with the interface. functionality not allowed in zeromq.
}

inline void FairMQMessageZMQ::CloseMessage()
{
  int rc = zmq_msg_close (fMessage);
  if (rc != 0) {
    LOG(ERROR) << "failed closing message, reason: " << zmq_strerror(errno);
  }
}

void FairMQMessageZMQ::Copy(FairMQMessage* msg)
{
  int rc = zmq_msg_copy (fMessage, (static_cast<FairMQMessageZMQ*>(msg)->fMessage));
  if (rc != 0) {
    LOG(ERROR) << "failed copying message, reason: " << zmq_strerror(errno);
  }
}

void FairMQMessageZMQ::CleanUp(void* data, void* hint)
{
  free (data);
}

FairMQMessageZMQ::~FairMQMessageZMQ()
{
  int rc = zmq_msg_close (fMessage);
  if (rc != 0) {
    LOG(ERROR) << "failed closing message with data, reason: " << zmq_strerror(errno);
  }
}