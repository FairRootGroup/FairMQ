/**
 * FairMQBinSink.h
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQPROTOSINK_H_
#define FAIRMQPROTOSINK_H_

#include "FairMQDevice.h"

struct Content
{
    double a;
    double b;
    int x;
    int y;
    int z;
};

class FairMQBinSink : public FairMQDevice
{
  public:
    FairMQBinSink();
    virtual ~FairMQBinSink();

  protected:
    virtual void Run();
};

#endif /* FAIRMQPROTOSINK_H_ */
