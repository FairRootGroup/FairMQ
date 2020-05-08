/********************************************************************************
 * Copyright (C) 2017-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQTransportFactory.h>
#include <fairmq/shmem/TransportFactory.h>
#include <zeromq/FairMQTransportFactoryZMQ.h>
#ifdef BUILD_OFI_TRANSPORT
#include <fairmq/ofi/TransportFactory.h>
#endif
#include <FairMQLogger.h>
#include <fairmq/tools/Unique.h>
#include <memory>
#include <string>

using namespace std;

FairMQTransportFactory::FairMQTransportFactory(const string& id)
    : fkId(id)
{}

auto FairMQTransportFactory::CreateTransportFactory(const string& type,
                                                    const string& id,
                                                    const fair::mq::ProgOptions* config)
    -> shared_ptr<FairMQTransportFactory>
{
    auto finalId = id;

    // Generate uuid if empty
    if (finalId == "") {
        finalId = fair::mq::tools::Uuid();
    }

    if (type == "zeromq") {
        return make_shared<FairMQTransportFactoryZMQ>(finalId, config);
    } else if (type == "shmem") {
        return make_shared<fair::mq::shmem::TransportFactory>(finalId, config);
    }
#ifdef BUILD_OFI_TRANSPORT
    else if (type == "ofi") {
        return make_shared<fair::mq::ofi::TransportFactory>(finalId, config);
    }
#endif /* BUILD_OFI_TRANSPORT */
    else {
        LOG(error) << "Unavailable transport requested: "
                   << "\"" << type << "\""
                   << ". Available are: "
                   << "\"zeromq\","
                   << "\"shmem\""
#ifdef BUILD_OFI_TRANSPORT
                   << ", and \"ofi\""
#endif /* BUILD_OFI_TRANSPORT */
                   << ". Exiting.";
        exit(EXIT_FAILURE);
    }
}
