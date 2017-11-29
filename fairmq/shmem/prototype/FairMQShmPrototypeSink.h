/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQShmPrototypeSink.h
 *
 * @since 2016-04-08
 * @author A. Rybalchenko
 */

#ifndef FAIRMQSHMPROTOTYPESINK_H_
#define FAIRMQSHMPROTOTYPESINK_H_

#include <atomic>

#include "FairMQDevice.h"

class FairMQShmPrototypeSink : public FairMQDevice
{
  public:
    FairMQShmPrototypeSink();
    virtual ~FairMQShmPrototypeSink();

    void Log(const int intervalInMs);

  protected:
    unsigned long long fBytesIn;
    unsigned long long fMsgIn;
    std::atomic<unsigned long long> fBytesInNew;
    std::atomic<unsigned long long> fMsgInNew;

    virtual void Init();
    virtual void Run();
};

#endif /* FAIRMQSHMPROTOTYPESINK_H_ */
