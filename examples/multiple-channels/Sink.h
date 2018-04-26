/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Sink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLEMULTIPLECHANNELSSINK_H
#define FAIRMQEXAMPLEMULTIPLECHANNELSSINK_H

#include "FairMQDevice.h"

namespace example_multiple_channels
{

class Sink : public FairMQDevice
{
  public:
    Sink();
    virtual ~Sink();

  protected:
    bool HandleBroadcast(FairMQMessagePtr&, int);
    bool HandleData(FairMQMessagePtr&, int);
    bool CheckIterations();
    virtual void InitTask();

  private:
    bool fReceivedData;
    bool fReceivedBroadcast;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

}

#endif /* FAIRMQEXAMPLEMULTIPLECHANNELSSINK_H */
