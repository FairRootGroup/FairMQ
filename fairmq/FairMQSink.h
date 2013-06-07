/*
 * FairMQSink.h
 *
 *  Created on: Jan 9, 2013
 *      Author: dklein
 */

#ifndef FAIRMQSINK_H_
#define FAIRMQSINK_H_

#include "Rtypes.h"
#include <pthread.h>

#include "FairMQDevice.h"



class FairMQSink: public FairMQDevice
{
  public:
    FairMQSink();
    virtual void Run();
    virtual ~FairMQSink();
};

#endif /* FAIRMQSINK_H_ */
