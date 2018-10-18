/********************************************************************************
 *    Copyright (C) 2018 CERN and copyright holders of ALICE O2 *
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the *
 *              GNU Lesser General Public Licence (LGPL) version 3, *
 *                  copied verbatim in the file "LICENSE" *
 ********************************************************************************/

/// @brief Memory allocators and interfaces related to managing memory via the
/// trasport layer
///
/// @author Mikolaj Krzewicki, mkrzewic@cern.ch

#include <fairmq/FairMQTransportFactory.h>
#include <fairmq/MemoryResources.h>

void *fair::mq::ChannelResource::do_allocate(std::size_t bytes, std::size_t /*alignment*/)
{
    return setMessage(factory->CreateMessage(bytes));
};

fair::mq::MessageResource::MessageResource(FairMQMessagePtr message)
: mUpstream{message->GetTransport()->GetMemoryResource()}
, mMessageSize{message->GetSize()}
, mMessageData{mUpstream ? mUpstream->setMessage(std::move(message))
  : throw std::runtime_error(
      "MessageResource::MessageResource message has no upstream resource set")}
{
}
