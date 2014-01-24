/**
 * FairMQPollerZMQ.h
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#ifndef FAIRMQPOLLERZMQ_H_
#define FAIRMQPOLLERZMQ_H_

#include <vector>

#include "FairMQPoller.h"
#include "FairMQSocket.h"

using std::vector;

class FairMQPollerZMQ : public FairMQPoller
{
  public:
    FairMQPollerZMQ(const vector<FairMQSocket*>& inputs);

    virtual void Poll(int timeout);
    virtual bool CheckInput(int index);

    virtual ~FairMQPollerZMQ();

  private:
    zmq_pollitem_t* items;
    int fNumItems;
};

#endif /* FAIRMQPOLLERZMQ_H_ */