/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQPOLLER_H_
#define FAIRMQPOLLER_H_

#include <memory>
#include <stdexcept>
#include <string>

class FairMQPoller
{
  public:
    virtual void Poll(const int timeout) = 0;
    virtual bool CheckInput(const int index) = 0;
    virtual bool CheckOutput(const int index) = 0;
    virtual bool CheckInput(const std::string& channelKey, const int index) = 0;
    virtual bool CheckOutput(const std::string& channelKey, const int index) = 0;

    virtual ~FairMQPoller() {};
};

using FairMQPollerPtr = std::unique_ptr<FairMQPoller>;

namespace fair::mq
{

using Poller = FairMQPoller;
using PollerPtr = FairMQPollerPtr;
struct PollerError : std::runtime_error { using std::runtime_error::runtime_error; };

} // namespace fair::mq

#endif /* FAIRMQPOLLER_H_ */
