/********************************************************************************
 * Copyright (C) 2017-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/TransportFactory.h>
#include <fairmq/shmem/TransportFactory.h>
#include <fairmq/zeromq/TransportFactory.h>
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
    else {
        LOG(error) << "Unavailable transport requested: "
                   << "\"" << type << "\""
                   << ". Available are: "
                   << "\"zeromq\","
                   << "\"shmem\""
                   << ". Exiting.";
        throw TransportFactoryError(tools::ToString("Unavailable transport requested: ", type));
    }
}

}   // namespace fair::mq
