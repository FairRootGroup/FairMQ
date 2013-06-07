/*
 * FairMQProcessor.h
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#ifndef FAIRMQPROCESSOR_H_
#define FAIRMQPROCESSOR_H_

#include "FairMQDevice.h"
#include "FairMQProcessorTask.h"
#include "Rtypes.h"


class FairMQProcessor: public FairMQDevice
{
  public:
    FairMQProcessor();
    virtual ~FairMQProcessor();
    void SetTask(FairMQProcessorTask* task);
    virtual void Init();
    virtual void Run();
  private:
    FairMQProcessorTask* fTask;
};

#endif /* FAIRMQPROCESSOR_H_ */
