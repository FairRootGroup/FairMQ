/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQPollerNN.h
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#ifndef FAIRMQPOLLERNN_H_
#define FAIRMQPOLLERNN_H_

#include <vector>

#include "FairMQPoller.h"
#include "FairMQSocket.h"

using std::vector;

class FairMQPollerNN : public FairMQPoller
{
  public:
    FairMQPollerNN(const vector<FairMQSocket*>& inputs);

    virtual void Poll(int timeout);
    virtual bool CheckInput(int index);

    virtual ~FairMQPollerNN();

  private:
    nn_pollfd* items;
    int fNumItems;
};

#endif /* FAIRMQPOLLERNN_H_ */