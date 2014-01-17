/**
 * FairMQBuffer.h
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQBUFFER_H_
#define FAIRMQBUFFER_H_

#include "FairMQDevice.h"


class FairMQBuffer: public FairMQDevice
{
  public:
    FairMQBuffer();
    virtual ~FairMQBuffer();
  protected:
    virtual void Run();
};

#endif /* FAIRMQBUFFER_H_ */
