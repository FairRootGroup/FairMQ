/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQTRANSPORTS_H_
#define FAIRMQTRANSPORTS_H_

#include <fairmq/Tools.h>
#include <string>
#include <unordered_map>

namespace FairMQ
{

enum class Transport
{
    DEFAULT,
    ZMQ,
    NN,
    SHM
};


static std::unordered_map<std::string, Transport> TransportTypes {
    { "default", Transport::DEFAULT },
    { "zeromq", Transport::ZMQ },
    { "nanomsg", Transport::NN },
    { "shmem", Transport::SHM }
};

}

namespace std
{

template<>
struct hash<FairMQ::Transport> : fair::mq::tools::HashEnum<FairMQ::Transport> {};

} /* namespace std */

#endif /* FAIRMQTRANSPORTS_H_ */
