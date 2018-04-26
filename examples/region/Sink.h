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

#ifndef FAIRMQEXAMPLEREGIONSINK_H
#define FAIRMQEXAMPLEREGIONSINK_H

#include <string>

#include "FairMQDevice.h"

namespace example_region
{

class Sink : public FairMQDevice
{
  public:
    Sink();
    virtual ~Sink();

  protected:
    virtual void Run();
    virtual void InitTask();

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

} // namespace example_region

#endif /* FAIRMQEXAMPLEREGIONSINK_H */
