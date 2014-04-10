/**
 * FairMQSink.h
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSINK_H_
#define FAIRMQSINK_H_

#include "FairMQDevice.h"

class FairMQSink : public FairMQDevice
{
  public:
    FairMQSink();
    virtual ~FairMQSink();

  protected:
    virtual void Run();
};

#endif /* FAIRMQSINK_H_ */
