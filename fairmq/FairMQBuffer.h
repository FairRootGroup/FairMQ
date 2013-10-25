/*
 * FairMQBuffer.h
 *
 *  Created on: Oct 25, 2012
 *      Author: dklein
 */

#ifndef FAIRMQBUFFER_H_
#define FAIRMQBUFFER_H_

#include "FairMQDevice.h"
#include "Rtypes.h"


class FairMQBuffer: public FairMQDevice
{
  public:
    FairMQBuffer();
    virtual ~FairMQBuffer();
  protected:
    virtual void Run();
};

#endif /* FAIRMQBUFFER_H_ */
