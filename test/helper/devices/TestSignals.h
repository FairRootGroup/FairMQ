/********************************************************************************
 *   Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH     *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_SIGNALS_H
#define FAIR_MQ_TEST_SIGNALS_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>

#include <iostream>
#include <csignal>

namespace fair::mq::test
{

class Signals : public FairMQDevice
{
  public:
    void Init() override
    {
        std::string state("Init");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }
    void Bind() override
    {
        std::string state("Bind");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }
    void Connect() override
    {
        std::string state("Connect");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }

    void InitTask() override
    {
        std::string state("InitTask");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }

    void PreRun() override
    {
        std::string state("PreRun");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }

    void Run() override
    {
        std::string state("Run");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }

    void PostRun() override
    {
        std::string state("PostRun");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }

    void ResetTask() override
    {
        std::string state("ResetTask");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }

    void Reset() override
    {
        std::string state("Reset");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "raising SIGINT from " << state << "()";
            raise(SIGINT);
        }
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_SIGNALS_H */
