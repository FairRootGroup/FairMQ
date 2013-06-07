/*
 * runMerger.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include "FairMQStandaloneMerger.h"
#include <sys/types.h>
#include <unistd.h>
#include "FairMQLogger.h"
#include <zmq.hpp>
#include <stdio.h>
#include <iostream>


int main(int argc, char** argv)
{
  if( argc != 12 ) {
    std::cout << "Usage: merger \tID numIoTreads\n" <<
              "\t\tconnectSocketType connectRcvBufferSize ConnectAddress\n" <<
              "\t\tconnectSocketType connectRcvBufferSize ConnectAddress\n" <<
              "\t\tbindSocketType bindSndBufferSize BindAddress\n" << std::endl;
    return 1;
  }

  pid_t pid = getpid();
  std::stringstream logmsg;
  logmsg << "PID: " << pid;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  FairMQStandaloneMerger* merger = new FairMQStandaloneMerger();
  merger->SetProperty(FairMQStandaloneMerger::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  merger->SetProperty(FairMQStandaloneMerger::NumIoThreads, numIoThreads);
  ++i;

  int numInputs = 2;
  merger->SetProperty(FairMQStandaloneMerger::NumInputs, numInputs);

  int numOutputs = 1;
  merger->SetProperty(FairMQStandaloneMerger::NumOutputs, numOutputs);

  merger->Init();

  int connectSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    connectSocketType = ZMQ_PULL;
  }
  merger->SetProperty(FairMQStandaloneMerger::ConnectSocketType, connectSocketType, 0);
  ++i;

  int connectRcvBufferSize;
  std::stringstream(argv[i]) >> connectRcvBufferSize;
  merger->SetProperty(FairMQStandaloneMerger::ConnectRcvBufferSize, connectRcvBufferSize, 0);
  ++i;

  merger->SetProperty(FairMQStandaloneMerger::ConnectAddress, argv[i], 0);
  ++i;

  connectSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    connectSocketType = ZMQ_PULL;
  }
  merger->SetProperty(FairMQStandaloneMerger::ConnectSocketType, connectSocketType, 1);
  ++i;

  std::stringstream(argv[i]) >> connectRcvBufferSize;
  merger->SetProperty(FairMQStandaloneMerger::ConnectRcvBufferSize, connectRcvBufferSize, 1);
  ++i;

  merger->SetProperty(FairMQStandaloneMerger::ConnectAddress, argv[i], 1);
  ++i;

  int bindSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    bindSocketType = ZMQ_PUSH;
  }
  merger->SetProperty(FairMQStandaloneMerger::BindSocketType, bindSocketType, 0);
  ++i;

  int bindSndBufferSize;
  std::stringstream(argv[i]) >> bindSndBufferSize;
  merger->SetProperty(FairMQStandaloneMerger::BindSndBufferSize, bindSndBufferSize, 0);
  ++i;

  merger->SetProperty(FairMQStandaloneMerger::BindAddress, argv[i], 0);
  ++i;


  merger->Bind();
  merger->Connect();
  merger->Run();

  exit(0);
}

