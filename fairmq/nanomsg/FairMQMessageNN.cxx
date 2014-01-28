/**
 * FairMQMessageNN.cxx
 *
 * @since 2013-12-05
 * @author: A. Rybalchenko
 */

#include <cstring>

#include <nanomsg/nn.h>

#include "FairMQMessageNN.h"
#include "FairMQLogger.h"


FairMQMessageNN::FairMQMessageNN() :
  fSize(0),
  fMessage(NULL)
{
}

FairMQMessageNN::FairMQMessageNN(size_t size)
{
  fMessage = nn_allocmsg(size, 0);
  if(!fMessage){
    stringstream logmsg;
    logmsg << "failed allocating message, reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
  fSize = size;
}

FairMQMessageNN::FairMQMessageNN(void* data, size_t size)
{
  fMessage = nn_allocmsg(size, 0);
  if(!fMessage){
    stringstream logmsg;
    logmsg << "failed allocating message, reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
  memcpy (fMessage, data, size);
  fSize = size;
}

void FairMQMessageNN::Rebuild()
{
  Clear();
  fSize = 0;
  fMessage = NULL;
}

void FairMQMessageNN::Rebuild(size_t size)
{
  Clear();
  fMessage = nn_allocmsg(size, 0);
  if(!fMessage){
    stringstream logmsg;
    logmsg << "failed allocating message, reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
  fSize = size;
}

void FairMQMessageNN::Rebuild(void* data, size_t size)
{
  Clear();
  fMessage = nn_allocmsg(size, 0);
  if(!fMessage){
    stringstream logmsg;
    logmsg << "failed allocating message, reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
  fSize = size;
}

void* FairMQMessageNN::GetMessage()
{
  return fMessage;
}

void* FairMQMessageNN::GetData()
{
  return fMessage;
}

size_t FairMQMessageNN::GetSize()
{
  return fSize;
}

void FairMQMessageNN::SetMessage(void* data, size_t size)
{
  fMessage = data;
  fSize = size;
}

void FairMQMessageNN::Copy(FairMQMessage* msg)
{
  if(fMessage){
    int rc = nn_freemsg(fMessage);
    if( rc < 0 ){
      stringstream logmsg;
      logmsg << "failed freeing message, reason: " << nn_strerror(errno);
      FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
    }
  }

  size_t size = msg->GetSize();

  fMessage = nn_allocmsg(size, 0);
  if(!fMessage){
    stringstream logmsg;
    logmsg << "failed allocating message, reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  }
  std::memcpy (fMessage, msg->GetMessage(), size);
  fSize = size;
}

void FairMQMessageNN::Clear()
{
  int rc = nn_freemsg(fMessage);
  if (rc < 0) {
    stringstream logmsg;
    logmsg << "failed freeing message, reason: " << nn_strerror(errno);
    FairMQLogger::GetInstance()->Log(FairMQLogger::ERROR, logmsg.str());
  } else {
    fMessage = NULL;
    fSize = 0;
  }
}

FairMQMessageNN::~FairMQMessageNN()
{
}
