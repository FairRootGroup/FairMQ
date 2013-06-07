/*
 * FairMQStandaloneMerger.h
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#ifndef FAIRMQSTANDALONEMERGER_H_
#define FAIRMQSTANDALONEMERGER_H_

#include "FairMQDevice.h"
#include "Rtypes.h"
#include "TString.h"


class FairMQStandaloneMerger: public FairMQDevice
{
  public:
    FairMQStandaloneMerger();
    virtual ~FairMQStandaloneMerger();
    virtual void Run();
};

#endif /* FAIRMQSTANDALONEMERGER_H_ */
