/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_ZMQ_COMMON_H
#define FAIR_MQ_ZMQ_COMMON_H

#include <fairmq/tools/Strings.h>
#include <stdexcept>
#include <string_view>
#include <zmq.h>

namespace fair::mq::zmq
{

struct Error : std::runtime_error { using std::runtime_error::runtime_error; };

/// Lookup table for various zmq constants
inline auto getConstant(std::string_view constant) -> int
{
    if (constant.empty()) { return 0; }
    if (constant == "sub") { return ZMQ_SUB; }
    if (constant == "pub") { return ZMQ_PUB; }
    if (constant == "xsub") { return ZMQ_XSUB; }
    if (constant == "xpub") { return ZMQ_XPUB; }
    if (constant == "push") { return ZMQ_PUSH; }
    if (constant == "pull") { return ZMQ_PULL; }
    if (constant == "req") { return ZMQ_REQ; }
    if (constant == "rep") { return ZMQ_REP; }
    if (constant == "dealer") { return ZMQ_DEALER; }
    if (constant == "router") { return ZMQ_ROUTER; }
    if (constant == "pair") { return ZMQ_PAIR; }

    if (constant == "snd-hwm") { return ZMQ_SNDHWM; }
    if (constant == "rcv-hwm") { return ZMQ_RCVHWM; }
    if (constant == "snd-size") { return ZMQ_SNDBUF; }
    if (constant == "rcv-size") { return ZMQ_RCVBUF; }
    if (constant == "snd-more") { return ZMQ_SNDMORE; }
    if (constant == "rcv-more") { return ZMQ_RCVMORE; }

    if (constant == "linger") { return ZMQ_LINGER; }
    if (constant == "no-block") { return ZMQ_DONTWAIT; }
    if (constant == "snd-more no-block") { return ZMQ_DONTWAIT|ZMQ_SNDMORE; }

    if (constant == "fd") { return ZMQ_FD; }
    if (constant == "events") { return ZMQ_EVENTS; }
    if (constant == "pollin") { return ZMQ_POLLIN; }
    if (constant == "pollout") { return ZMQ_POLLOUT; }

    throw Error(tools::ToString("getConstant called with an invalid argument: ", constant));
}

} // namespace fair::mq::zmq

#endif /* FAIR_MQ_ZMQ_COMMON_H */
