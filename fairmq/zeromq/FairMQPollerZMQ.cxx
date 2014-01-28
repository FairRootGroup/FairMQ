/**
 * FairMQPollerZMQ.cxx
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#include <zmq.h>

#include "FairMQPollerZMQ.h"

FairMQPollerZMQ::FairMQPollerZMQ(const vector<FairMQSocket*>& inputs)
{
  fNumItems = inputs.size();
  items = new zmq_pollitem_t[fNumItems];

  for (int i = 0; i < fNumItems; i++) {
    items[i].socket = inputs.at(i)->GetSocket();
    items[i].fd = 0;
    items[i].events = ZMQ_POLLIN;
    items[i].revents = 0;
  }
}

void FairMQPollerZMQ::Poll(int timeout)
{
  zmq_poll(items, fNumItems, timeout);
}

bool FairMQPollerZMQ::CheckInput(int index)
{
  if (items[index].revents & ZMQ_POLLIN) 
    return true;

  return false;
}

FairMQPollerZMQ::~FairMQPollerZMQ()
{
  if (items != NULL) delete [] items;
}
