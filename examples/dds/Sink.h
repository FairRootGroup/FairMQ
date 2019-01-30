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

#ifndef FAIRMQEXAMPLEDDSSINK_H
#define FAIRMQEXAMPLEDDSSINK_H

#include "FairMQDevice.h"

namespace example_dds
{

class Sink : public FairMQDevice
{
  public:
    Sink();
    virtual ~Sink();
    void InitTask() override;

  protected:
    bool HandleData(FairMQMessagePtr&, int);

  private:
    uint64_t fIterations;
    uint64_t fCounter;
};

} // namespace example_dds

#endif /* FAIRMQEXAMPLEDDSSINK_H */
