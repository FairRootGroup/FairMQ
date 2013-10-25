/*
 * FairMQProxy.h
 *
 *  Created on: Oct 2, 2013
 *      Author: A. Rybalchenko
 */

#ifndef FAIRMQPROXY_H_
#define FAIRMQPROXY_H_

#include "FairMQDevice.h"
#include "Rtypes.h"
#include "TString.h"


class FairMQProxy: public FairMQDevice
{
  public:
    FairMQProxy();
    virtual ~FairMQProxy();
  protected:
    virtual void Run();
};

#endif /* FAIRMQPROXY_H_ */
