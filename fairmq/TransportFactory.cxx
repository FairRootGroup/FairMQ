/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/TransportFactory.h>
#include <fairmq/shmem/TransportFactory.h>
#include <fairmq/zeromq/TransportFactory.h>
#ifdef BUILD_OFI_TRANSPORT
#include <fairmq/ofi/TransportFactory.h>
#endif
#include <fairlogger/Logger.h>
#include <fairmq/Tools.h>
#include <memory>
#include <string>
#include <utility>   // move

using namespace std;

namespace fair::mq {

auto TransportFactory::CreateTransportFactory(const string& type,
                                              const string& id,
                                              const ProgOptions* config)
    -> shared_ptr<TransportFactory>
{
    auto finalId = id;

    // Generate uuid if empty
    if (finalId.empty()) {
        finalId = tools::Uuid();
    }

    if (type == "zeromq") {
        return make_shared<zmq::TransportFactory>(finalId, config);
    } else if (type == "shmem") {
        return make_shared<shmem::TransportFactory>(finalId, config);
    }
#ifdef BUILD_OFI_TRANSPORT
    else if (type == "ofi") {
        return make_shared<ofi::TransportFactory>(finalId, config);
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
        throw TransportFactoryError(tools::ToString("Unavailable transport requested: ", type));
    }
}

}   // namespace fair::mq
