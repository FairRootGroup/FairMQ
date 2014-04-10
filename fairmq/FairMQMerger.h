/**
 * FairMQMerger.h
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQMERGER_H_
#define FAIRMQMERGER_H_

#include "FairMQDevice.h"

class FairMQMerger : public FairMQDevice
{
  public:
    FairMQMerger();
    virtual ~FairMQMerger();

  protected:
    virtual void Run();
};

#endif /* FAIRMQMERGER_H_ */
