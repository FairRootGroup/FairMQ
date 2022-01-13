/********************************************************************************
 *   Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH     *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_ERROR_STATE_H
#define FAIR_MQ_TEST_ERROR_STATE_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>

#include <iostream>

namespace fair::mq::test
{

class ErrorState : public FairMQDevice
{
  public:
    void Init() override
    {
        std::string state("Init");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }

    void Bind() override
    {
        std::string state("Bind");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }

    void Connect() override
    {
        std::string state("Connect");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }

    void InitTask() override
    {
        std::string state("InitTask");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }

    void PreRun() override
    {
        std::string state("PreRun");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }

    void Run() override
    {
        std::string state("Run");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }

    void PostRun() override
    {
        std::string state("PostRun");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }

    void ResetTask() override
    {
        std::string state("ResetTask");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }

    void Reset() override
    {
        std::string state("Reset");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeState(fair::mq::Transition::ErrorFound);
        }
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_ERROR_STATE_H */
