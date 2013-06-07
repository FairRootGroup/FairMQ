/*
 * FairMQProcessorTask.h
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#ifndef FAIRMQPROCESSORTASK_H_
#define FAIRMQPROCESSORTASK_H_

#include <vector>
#include "FairMQMessage.h"
#include "FairTask.h"


class FairMQProcessorTask : public FairTask
{
  public:
    FairMQProcessorTask();
    virtual ~FairMQProcessorTask();
    virtual void Exec(FairMQMessage* msg, Option_t* opt) = 0;
};

#endif /* FAIRMQPROCESSORTASK_H_ */
