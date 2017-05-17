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
#include <memory>

auto FairMQTransportFactory::CreateTransportFactory(const std::string& type) -> std::shared_ptr<FairMQTransportFactory>
{
    if (type == "zeromq")
    {
        return std::make_shared<FairMQTransportFactoryZMQ>();
    }
    else if (type == "shmem")
    {
        return std::make_shared<FairMQTransportFactorySHM>();
    }
#ifdef NANOMSG_FOUND
    else if (type == "nanomsg")
    {
        return std::make_shared<FairMQTransportFactoryNN>();
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
