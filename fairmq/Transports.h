/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TRANSPORTS_H
#define FAIR_MQ_TRANSPORTS_H

#include <fairmq/Tools.h>

#include <memory>
#include <string>
#include <unordered_map>

/// TODO deprecate this namespace
namespace FairMQ
{

enum class Transport
{
    DEFAULT,
    ZMQ,
#ifdef NANOMSG_FOUND
    NN,
#endif
    SHM,
    OFI
};


static std::unordered_map<std::string, Transport> TransportTypes {
    { "default", Transport::DEFAULT },
    { "zeromq", Transport::ZMQ },
#ifdef NANOMSG_FOUND
    { "nanomsg", Transport::NN },
#endif
    { "shmem", Transport::SHM },
    { "ofi", Transport::OFI }
};

}

namespace fair
{
namespace mq
{

using Transport = ::FairMQ::Transport;
using ::FairMQ::TransportTypes;

} /* namespace mq */
} /* namespace fair */

namespace std
{

template<>
struct hash<FairMQ::Transport> : fair::mq::tools::HashEnum<FairMQ::Transport> {};

} /* namespace std */

#endif /* FAIR_MQ_TRANSPORTS_H */
