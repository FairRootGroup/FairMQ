/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
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

using namespace std;

class FairMQPollerZMQ : public FairMQPoller
{
  public:
    FairMQPollerZMQ(const vector<FairMQSocket*>& inputs);

    virtual void Poll(int timeout);
    virtual bool CheckInput(int index);
    virtual bool CheckOutput(int index);

    virtual ~FairMQPollerZMQ();

  private:
    zmq_pollitem_t* items;
    int fNumItems;

    /// Copy Constructor
    FairMQPollerZMQ(const FairMQPollerZMQ&);
    FairMQPollerZMQ operator=(const FairMQPollerZMQ&);
};

#endif /* FAIRMQPOLLERZMQ_H_ */