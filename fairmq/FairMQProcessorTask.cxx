/*
 * FairMQProcessorTask.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include "FairMQProcessorTask.h"


FairMQProcessorTask::FairMQProcessorTask()
{
}

FairMQProcessorTask::~FairMQProcessorTask()
{
}

void FairMQProcessorTask::ClearOutput(void* data, void* hint)
{
  free (data);
}