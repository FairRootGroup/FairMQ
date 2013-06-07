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
  private:
  public:
    FairMQBuffer();
    virtual void Run();
    virtual ~FairMQBuffer();
};

#endif /* FAIRMQBUFFER_H_ */
