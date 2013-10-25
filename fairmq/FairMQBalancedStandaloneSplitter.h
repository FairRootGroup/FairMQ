/*
 * FairMQBalancedStandaloneSplitter.h
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#ifndef FAIRMQBALANCEDSTANDALONESPLITTER_H_
#define FAIRMQBALANCEDSTANDALONESPLITTER_H_

#include "FairMQDevice.h"
#include "Rtypes.h"


class FairMQBalancedStandaloneSplitter: public FairMQDevice
{
  public:
    FairMQBalancedStandaloneSplitter();
    virtual ~FairMQBalancedStandaloneSplitter();
  protected:
    virtual void Run();
};

#endif /* FAIRMQBALANCEDSTANDALONESPLITTER_H_ */
