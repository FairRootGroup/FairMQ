/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_POLLER_H
#define FAIR_MQ_POLLER_H

#include <memory>
#include <stdexcept>
#include <string>

namespace fair::mq {

struct Poller
{
    virtual void Poll(const int timeout) = 0;
    virtual bool CheckInput(const int index) = 0;
    virtual bool CheckOutput(const int index) = 0;
    virtual bool CheckInput(const std::string& channelKey, const int index) = 0;
    virtual bool CheckOutput(const std::string& channelKey, const int index) = 0;

    virtual ~Poller() = default;
};

using PollerPtr = std::unique_ptr<Poller>;

struct PollerError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}   // namespace fair::mq

using FairMQPoller [[deprecated("Use fair::mq::Poller")]] = fair::mq::Poller;
using FairMQPollerPtr [[deprecated("Use fair::mq::PollerPtr")]] = fair::mq::PollerPtr;

#endif   // FAIR_MQ_POLLER_H
