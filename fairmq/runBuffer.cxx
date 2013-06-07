/*
 * runBuffer.cxx
 *
 *  Created on: Oct 26, 2012
 *      Author: dklein
 */

#include "FairMQBuffer.h"
#include <sys/types.h>
#include <unistd.h>
#include "FairMQLogger.h"
#include <zmq.hpp>
#include <stdio.h>
#include <iostream>


int main(int argc, char** argv)
{
  if( argc != 9 ) {
    std::cout << "Usage: buffer \tID numIoTreads\n" <<
              "\t\tconnectSocketType connectRcvBufferSize ConnectAddress\n" <<
              "\t\tbindSocketType bindSndBufferSize BindAddress\n" << std::endl;
    return 1;
  }

  pid_t pid = getpid();
  std::stringstream logmsg;
  logmsg << "PID: " << pid;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  FairMQBuffer* buffer = new FairMQBuffer();
  buffer->SetProperty(FairMQBuffer::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  buffer->SetProperty(FairMQBuffer::NumIoThreads, numIoThreads);
  ++i;

  int numInputs = 1;
  buffer->SetProperty(FairMQBuffer::NumInputs, numInputs);

  int numOutputs = 1;
  buffer->SetProperty(FairMQBuffer::NumOutputs, numOutputs);

  buffer->Init();

  int connectSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    connectSocketType = ZMQ_PULL;
  }
  buffer->SetProperty(FairMQBuffer::ConnectSocketType, connectSocketType, 0);
  ++i;

  int connectRcvBufferSize;
  std::stringstream(argv[i]) >> connectRcvBufferSize;
  buffer->SetProperty(FairMQBuffer::ConnectRcvBufferSize, connectRcvBufferSize, 0);
  ++i;

  buffer->SetProperty(FairMQBuffer::ConnectAddress, argv[i], 0);
  ++i;

  int bindSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    bindSocketType = ZMQ_PUSH;
  }
  buffer->SetProperty(FairMQBuffer::BindSocketType, bindSocketType, 0);
  ++i;

  int bindSndBufferSize;
  std::stringstream(argv[i]) >> bindSndBufferSize;
  buffer->SetProperty(FairMQBuffer::BindSndBufferSize, bindSndBufferSize, 0);
  ++i;

  buffer->SetProperty(FairMQBuffer::BindAddress, argv[i], 0);
  ++i;


  buffer->Bind();
  buffer->Connect();
  buffer->Run();

  exit(0);
}

