/**
 * FairMQContext.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQCONTEXT_H_
#define FAIRMQCONTEXT_H_

#include <zmq.h>

class FairMQContext
{
  public:
    FairMQContext(int numIoThreads);
    virtual ~FairMQContext();
    void* GetContext();
    void Close();

  private:
    void* fContext;
};

#endif /* FAIRMQCONTEXT_H_ */
