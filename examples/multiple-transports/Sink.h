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

#ifndef FAIRMQEXAMPLEMULTIPLETRANSPORTSSINK_H
#define FAIRMQEXAMPLEMULTIPLETRANSPORTSSINK_H

#include "FairMQDevice.h"

namespace example_multiple_transports
{

class Sink : public FairMQDevice
{
  public:
    Sink();
    virtual ~Sink();

  protected:
    virtual void InitTask();
    bool HandleData1(FairMQMessagePtr&, int);
    bool HandleData2(FairMQMessagePtr&, int);
    bool CheckIterations();

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations1;
    uint64_t fNumIterations2;
    bool fReceived1;
    bool fReceived2;
};

} // namespace example_multiple_transports

#endif /* FAIRMQEXAMPLEMULTIPLETRANSPORTSSINK_H */
