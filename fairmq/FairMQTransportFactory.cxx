/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
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
#include <FairMQLogger.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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
        finalId = boost::uuids::to_string(boost::uuids::random_generator()());
    }

    if (type == "zeromq")
    {
        return std::make_shared<FairMQTransportFactoryZMQ>(finalId, config);
    }
    else if (type == "shmem")
    {
        return std::make_shared<FairMQTransportFactorySHM>(finalId, config);
    }
#ifdef NANOMSG_FOUND
    else if (type == "nanomsg")
    {
        return std::make_shared<FairMQTransportFactoryNN>(finalId, config);
    }
#endif /* NANOMSG_FOUND */
    else
    {
        LOG(ERROR) << "Unavailable transport requested: " << "\"" << type << "\"" << ". Available are: "
                   << "\"zeromq\""
                   << "\"shmem\""
#ifdef NANOMSG_FOUND
                   << ", \"nanomsg\""
#endif /* NANOMSG_FOUND */
                   << ". Exiting.";
        exit(EXIT_FAILURE);
    }
}
