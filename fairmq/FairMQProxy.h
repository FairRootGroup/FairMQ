/**
 * FairMQProxy.h
 *
 * @since 2013-10-02
 * @author A. Rybalchenko
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
