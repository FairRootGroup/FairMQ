/*
 * runSink.cxx
 *
 *  Created on: Jan 21, 2013
 *      Author: dklein
 */

#include "FairMQSink.h"
#include <sys/types.h>
#include <unistd.h>
#include "FairMQLogger.h"
#include <zmq.hpp>
#include <stdio.h>
#include <iostream>


int main(int argc, char** argv)
{
  if( argc != 6 ) {
    std::cout << "Usage: sink \tID numIoTreads\n" <<
              "\t\tconnectSocketType connectRcvBufferSize ConnectAddress\n" << std::endl;
    return 1;
  }

  pid_t pid = getpid();
  std::stringstream logmsg;
  logmsg << "PID: " << pid;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  FairMQSink* sink = new FairMQSink();
  sink->SetProperty(FairMQSink::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  sink->SetProperty(FairMQSink::NumIoThreads, numIoThreads);
  ++i;

  int numInputs = 1;
  sink->SetProperty(FairMQSink::NumInputs, numInputs);

  int numOutputs = 0;
  sink->SetProperty(FairMQSink::NumOutputs, numOutputs);

  sink->Init();

  int connectSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    connectSocketType = ZMQ_PULL;
  }
  sink->SetProperty(FairMQSink::ConnectSocketType, connectSocketType, 0);
  ++i;

  int connectRcvBufferSize;
  std::stringstream(argv[i]) >> connectRcvBufferSize;
  sink->SetProperty(FairMQSink::ConnectRcvBufferSize, connectRcvBufferSize, 0);
  ++i;

  sink->SetProperty(FairMQSink::ConnectAddress, argv[i], 0);
  ++i;


  sink->Bind();
  sink->Connect();
  sink->Run();

  exit(0);
}

