/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Sampler.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE1N1SAMPLER_H_
#define FAIRMQEXAMPLE1N1SAMPLER_H_

#include <string>

#include "FairMQDevice.h"

namespace example_1_n_1
{

class Sampler : public FairMQDevice
{
  public:
    Sampler();
    virtual ~Sampler();

  protected:
    std::string fText;
    uint64_t fMaxIterations;
    uint64_t fNumIterations;

    virtual void InitTask();
    virtual bool ConditionalRun();
};

} // namespace example_1_n_1

#endif /* FAIRMQEXAMPLE1N1SAMPLER_H_ */
