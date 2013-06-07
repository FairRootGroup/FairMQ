/*
 * runSplitter.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include "FairMQBalancedStandaloneSplitter.h"
#include <sys/types.h>
#include <unistd.h>
#include "FairMQLogger.h"
#include <zmq.hpp>
#include <stdio.h>
#include <iostream>


int main(int argc, char** argv)
{
  if( argc != 12 ) {
    std::cout << "Usage: splitter \tID numIoTreads\n" <<
              "\t\tconnectSocketType connectRcvBufferSize ConnectAddress\n" <<
              "\t\tbindSocketType bindSndBufferSize BindAddress\n" <<
              "\t\tbindSocketType bindSndBufferSize BindAddress\n" << std::endl;
    return 1;
  }

  pid_t pid = getpid();
  std::stringstream logmsg;
  logmsg << "PID: " << pid;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  FairMQBalancedStandaloneSplitter* splitter = new FairMQBalancedStandaloneSplitter();
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::NumIoThreads, numIoThreads);
  ++i;

  int numInputs = 1;
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::NumInputs, numInputs);

  int numOutputs = 2;
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::NumOutputs, numOutputs);

  splitter->Init();

  int connectSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    connectSocketType = ZMQ_PULL;
  }
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::ConnectSocketType, connectSocketType, 0);
  ++i;

  int connectRcvBufferSize;
  std::stringstream(argv[i]) >> connectRcvBufferSize;
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::ConnectRcvBufferSize, connectRcvBufferSize, 0);
  ++i;

  splitter->SetProperty(FairMQBalancedStandaloneSplitter::ConnectAddress, argv[i], 0);
  ++i;

  int bindSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    bindSocketType = ZMQ_PUSH;
  }
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::BindSocketType, bindSocketType, 0);
  ++i;

  int bindSndBufferSize;
  std::stringstream(argv[i]) >> bindSndBufferSize;
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::BindSndBufferSize, bindSndBufferSize, 0);
  ++i;

  splitter->SetProperty(FairMQBalancedStandaloneSplitter::BindAddress, argv[i], 0);
  ++i;

  bindSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    bindSocketType = ZMQ_PUSH;
  }
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::BindSocketType, bindSocketType, 1);
  ++i;

  std::stringstream(argv[i]) >> bindSndBufferSize;
  splitter->SetProperty(FairMQBalancedStandaloneSplitter::BindSndBufferSize, bindSndBufferSize, 1);
  ++i;

  splitter->SetProperty(FairMQBalancedStandaloneSplitter::BindAddress, argv[i], 1);
  ++i;


  splitter->Bind();
  splitter->Connect();
  splitter->Run();

  exit(0);
}

