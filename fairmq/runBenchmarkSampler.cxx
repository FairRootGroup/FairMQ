/*
 * runBenchmarkSampler.cxx
 *
 *  Created on: Apr 23, 2013
 *      Author: dklein
 */

#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include "FairMQLogger.h"
#include <zmq.hpp>
#include <stdio.h>
#include <iostream>
#include "FairMQBenchmarkSampler.h"


int main(int argc, char** argv)
{
  if( argc != 8 ) {
    std::cout << "Usage: bsampler ID eventSize eventRate numIoTreads\n" <<
              "\t\tbindSocketType bindSndBufferSize BindAddress\n" << std::endl;
    return 1;
  }

  pid_t pid = getpid();
  std::stringstream logmsg;
  logmsg << "PID: " << pid;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  FairMQBenchmarkSampler* sampler = new FairMQBenchmarkSampler();

  sampler->SetProperty(FairMQBenchmarkSampler::Id, argv[i]);
  ++i;

  int eventSize;
  std::stringstream(argv[i]) >> eventSize;
  sampler->SetProperty(FairMQBenchmarkSampler::EventSize, eventSize);
  ++i;

  int eventRate;
  std::stringstream(argv[i]) >> eventRate;
  sampler->SetProperty(FairMQBenchmarkSampler::EventRate, eventRate);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  sampler->SetProperty(FairMQBenchmarkSampler::NumIoThreads, numIoThreads);
  ++i;

  int numInputs = 0;
  sampler->SetProperty(FairMQBenchmarkSampler::NumInputs, numInputs);

  int numOutputs = 1;
  sampler->SetProperty(FairMQBenchmarkSampler::NumOutputs, numOutputs);

  sampler->Init();

  int bindSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    bindSocketType = ZMQ_PUSH;
  }
  sampler->SetProperty(FairMQBenchmarkSampler::BindSocketType, bindSocketType, 0);
  ++i;

  int bindSndBufferSize;
  std::stringstream(argv[i]) >> bindSndBufferSize;
  sampler->SetProperty(FairMQBenchmarkSampler::BindSndBufferSize, bindSndBufferSize, 0);
  ++i;

  sampler->SetProperty(FairMQBenchmarkSampler::BindAddress, argv[i], 0);
  ++i;


  sampler->Bind();
  sampler->Connect();
  sampler->Run();

  exit(0);
}

