/********************************************************************************
 * Copyright (C) 2021-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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
    Poller() = default;
    Poller(const Poller&) = delete;
    Poller(Poller&&) = delete;
    Poller& operator=(const Poller&) = delete;
    Poller& operator=(Poller&&) = delete;

    virtual void Poll(int timeout) = 0;
    virtual bool CheckInput(int index) = 0;
    virtual bool CheckOutput(int index) = 0;
    virtual bool CheckInput(const std::string& channelKey, int index) = 0;
    virtual bool CheckOutput(const std::string& channelKey, int index) = 0;

    virtual ~Poller() = default;
};

using PollerPtr = std::unique_ptr<Poller>;

struct PollerError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}   // namespace fair::mq

#endif   // FAIR_MQ_POLLER_H
