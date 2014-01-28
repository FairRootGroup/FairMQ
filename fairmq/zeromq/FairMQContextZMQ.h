/**
 * FairMQContext.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQCONTEXTZMQ_H_
#define FAIRMQCONTEXTZMQ_H_

#include <zmq.h>

class FairMQContextZMQ
{
  public:
    FairMQContextZMQ(int numIoThreads);
    virtual ~FairMQContextZMQ();
    void* GetContext();
    void Close();

  private:
    void* fContext;
};

#endif /* FAIRMQCONTEXTZMQ_H_ */
