/**
 * FairMQProcessor.h
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQPROCESSOR_H_
#define FAIRMQPROCESSOR_H_

#include "FairMQDevice.h"
#include "FairMQProcessorTask.h"


class FairMQProcessor: public FairMQDevice
{
  public:
    FairMQProcessor();
    virtual ~FairMQProcessor();
    void SetTask(FairMQProcessorTask* task);
  protected:
    virtual void Init();
    virtual void Run();
  private:
    FairMQProcessorTask* fProcessorTask;
};

#endif /* FAIRMQPROCESSOR_H_ */
