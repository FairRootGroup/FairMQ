/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_FWDDECLS_H
#define FAIR_MQ_FWDDECLS_H

#include <fairmq/ProgOptionsFwd.h>

namespace fair::mq {

class Channel;
class Device;
class MemoryResource;
class Message;
class Parts;
class Poller;
class RegionBlock;
class RegionConfig;
class RegionInfo;
class Socket;
class TransportFactory;
class UnmanagedRegion;

using FairMQMemoryResource = MemoryResource;

}   // namespace fair::mq

using FairMQChannel = fair::mq::Channel;
using FairMQDevice = fair::mq::Device;
using FairMQMessage = fair::mq::Message;
using FairMQParts = fair::mq::Parts;
using FairMQPoller = fair::mq::Poller;
using FairMQRegionBlock = fair::mq::RegionBlock;
using FairMQRegionConfig = fair::mq::RegionConfig;
using FairMQRegionInfo = fair::mq::RegionInfo;
using FairMQSocket = fair::mq::Socket;
using FairMQTransportFactory = fair::mq::TransportFactory;
using FairMQUnmanagedRegion = fair::mq::UnmanagedRegion;

#endif   // FAIR_MQ_FWDDECLS_H
