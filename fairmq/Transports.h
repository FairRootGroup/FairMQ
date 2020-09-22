/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TRANSPORTS_H
#define FAIR_MQ_TRANSPORTS_H

#include <fairmq/tools/CppSTL.h>
#include <fairmq/tools/Strings.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace fair
{
namespace mq
{

enum class Transport
{
    DEFAULT,
    ZMQ,
    SHM,
    OFI
};

struct TransportError : std::runtime_error { using std::runtime_error::runtime_error; };

} /* namespace mq */
} /* namespace fair */

namespace std
{

template<>
struct hash<fair::mq::Transport> : fair::mq::tools::HashEnum<fair::mq::Transport> {};

} /* namespace std */

namespace fair
{
namespace mq
{

static std::unordered_map<std::string, Transport> TransportTypes {
    { "default", Transport::DEFAULT },
    { "zeromq", Transport::ZMQ },
    { "shmem", Transport::SHM },
    { "ofi", Transport::OFI }
};

static std::unordered_map<Transport, std::string> TransportNames {
    { Transport::DEFAULT, "default" },
    { Transport::ZMQ, "zeromq" },
    { Transport::SHM, "shmem" },
    { Transport::OFI, "ofi" }
};

inline std::string TransportName(Transport transport)
{
    return TransportNames[transport];
}

inline Transport TransportType(const std::string& transport)
try {
    return TransportTypes.at(transport);
} catch (std::out_of_range&) {
    throw TransportError(tools::ToString("Unknown transport provided: ", transport));
}

} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TRANSPORTS_H */
