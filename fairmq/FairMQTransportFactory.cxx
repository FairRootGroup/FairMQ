/********************************************************************************
 * Copyright (C) 2017-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQTransportFactory.h>
#include <zeromq/FairMQTransportFactoryZMQ.h>
#include <shmem/FairMQTransportFactorySHM.h>
#ifdef NANOMSG_FOUND
#include <nanomsg/FairMQTransportFactoryNN.h>
#endif /* NANOMSG_FOUND */
#ifdef BUILD_OFI_TRANSPORT
#include <fairmq/ofi/TransportFactory.h>
#endif
#include <FairMQLogger.h>
#include <fairmq/Tools.h>

#include <memory>
#include <string>
#include <sstream>

FairMQTransportFactory::FairMQTransportFactory(const std::string& id)
    : fkId(id)
{
}

auto FairMQTransportFactory::CreateTransportFactory(const std::string& type, const std::string& id, const FairMQProgOptions* config) -> std::shared_ptr<FairMQTransportFactory>
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
        return make_shared<FairMQTransportFactorySHM>(finalId, config);
    }
#ifdef NANOMSG_FOUND
    else if (type == "nanomsg")
    {
        return make_shared<FairMQTransportFactoryNN>(finalId, config);
    }
#endif /* NANOMSG_FOUND */
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
#ifdef NANOMSG_FOUND
                   << ", \"nanomsg\""
#endif /* NANOMSG_FOUND */
#ifdef BUILD_OFI_TRANSPORT
                   << ", and \"ofi\""
#endif /* BUILD_OFI_TRANSPORT */
                   << ". Exiting.";
        exit(EXIT_FAILURE);
    }
}
