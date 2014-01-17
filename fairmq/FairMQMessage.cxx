/**
 * FairMQMessage.cxx
 *
 * @since 2012-12-05
 * @author: D. Klein, A. Rybalchenko
 */

#include <cstdlib>

#include "FairMQMessage.h"
#include "FairMQLogger.h"


FairMQMessage::FairMQMessage()
{
  int rc = zmq_msg_init (&fMessage);
  if (rc != 0){
    std::stringstream logmsg;
    logmsg << "failed initializing message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

FairMQMessage::FairMQMessage(size_t size)
{
  int rc = zmq_msg_init_size (&fMessage, size);
  if (rc != 0){
    std::stringstream logmsg;
    logmsg << "failed initializing message with size, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

FairMQMessage::FairMQMessage(void* data, size_t size)
{
  int rc = zmq_msg_init_data (&fMessage, data, size, &CleanUp, NULL);
  if (rc != 0){
    std::stringstream logmsg;
    logmsg << "failed initializing message with data, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

FairMQMessage::~FairMQMessage()
{
  int rc = zmq_msg_close (&fMessage);
  if (rc != 0){
    std::stringstream logmsg;
    logmsg << "failed closing message with data, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

void FairMQMessage::Rebuild(void* data, size_t size)
{
  int rc = zmq_msg_close (&fMessage);
  if (rc != 0) {
    std::stringstream logmsg;
    logmsg << "failed closing message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }

  rc = zmq_msg_init_data (&fMessage, data, size, &CleanUp, NULL);
  if (rc != 0) {
    std::stringstream logmsg2;
    logmsg2 << "failed initializing message with data, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg2.str());
  }
}

zmq_msg_t* FairMQMessage::GetMessage()
{
  return &fMessage;
}

void* FairMQMessage::GetData()
{
  return zmq_msg_data (&fMessage);
}

size_t FairMQMessage::GetSize()
{
  return zmq_msg_size (&fMessage);
}

void FairMQMessage::Copy(FairMQMessage* msg)
{
  int rc = zmq_msg_copy (&fMessage, &(msg->fMessage));
  if (rc != 0) {
    std::stringstream logmsg;
    logmsg << "failed copying message, reason: " << zmq_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
}

void FairMQMessage::CleanUp(void* data, void* hint)
{
  free (data);
}
