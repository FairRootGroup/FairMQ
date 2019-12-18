/********************************************************************************
 * Copyright (C) 2017-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQTransportFactory.h>
#include <zeromq/FairMQTransportFactoryZMQ.h>
#include <fairmq/shmem/TransportFactory.h>
#ifdef BUILD_NANOMSG_TRANSPORT
#include <nanomsg/FairMQTransportFactoryNN.h>
#endif /* BUILD_NANOMSG_TRANSPORT */
#ifdef BUILD_OFI_TRANSPORT
#include <fairmq/ofi/TransportFactory.h>
#endif
#include <FairMQLogger.h>
#include <fairmq/tools/Unique.h>

#include <memory>
#include <string>

FairMQTransportFactory::FairMQTransportFactory(const std::string& id)
    : fkId(id)
{
}

auto FairMQTransportFactory::CreateTransportFactory(const std::string& type, const std::string& id, const fair::mq::ProgOptions* config) -> std::shared_ptr<FairMQTransportFactory>
{
    using namespace std;

    auto finalId = id;

    // Generate uuid if empty
    if (finalId == "")
    {
        finalId = fair::mq::tools::Uuid();
    }

    if (type == "zeromq")
    {
        return make_shared<FairMQTransportFactoryZMQ>(finalId, config);
    }
    else if (type == "shmem")
    {
        return make_shared<fair::mq::shmem::TransportFactory>(finalId, config);
    }
#ifdef BUILD_NANOMSG_TRANSPORT
    else if (type == "nanomsg")
    {
        return make_shared<FairMQTransportFactoryNN>(finalId, config);
    }
#endif /* BUILD_NANOMSG_TRANSPORT */
#ifdef BUILD_OFI_TRANSPORT
    else if (type == "ofi")
    {
        return make_shared<fair::mq::ofi::TransportFactory>(finalId, config);
    }
#endif /* BUILD_OFI_TRANSPORT */
    else
    {
        LOG(error) << "Unavailable transport requested: " << "\"" << type << "\"" << ". Available are: "
                   << "\"zeromq\""
                   << "\"shmem\""
#ifdef BUILD_NANOMSG_TRANSPORT
                   << ", \"nanomsg\""
#endif /* BUILD_NANOMSG_TRANSPORT */
#ifdef BUILD_OFI_TRANSPORT
                   << ", and \"ofi\""
#endif /* BUILD_OFI_TRANSPORT */
                   << ". Exiting.";
        exit(EXIT_FAILURE);
    }
}
