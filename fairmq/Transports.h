/********************************************************************************
 * Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TRANSPORTS_H
#define FAIR_MQ_TRANSPORTS_H

#include <fairmq/tools/Strings.h>
#include <fairmq/TransportEnum.h>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace fair::mq {

struct TransportError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

static const std::unordered_map<std::string, Transport> TransportTypes{
    {"default", Transport::DEFAULT},
    {"zeromq", Transport::ZMQ},
    {"shmem", Transport::SHM}
};

static const std::unordered_map<Transport, std::string> TransportNames{
    {Transport::DEFAULT, "default"},
    {Transport::ZMQ, "zeromq"},
    {Transport::SHM, "shmem"}
};

inline std::string TransportName(Transport transport) { return TransportNames.at(transport); }

inline Transport TransportType(const std::string& transport)
try {
    return TransportTypes.at(transport);
} catch (std::out_of_range&) {
    throw TransportError(tools::ToString("Unknown transport provided: ", transport));
}

inline std::ostream& operator<<(std::ostream& os, const Transport& transport)
{
    return os << TransportName(transport);
}

inline auto GetEnabledTransports() -> std::vector<Transport>
{
    return {Transport::ZMQ, Transport::SHM};
}

}   // namespace fair::mq

#endif /* FAIR_MQ_TRANSPORTS_H */
