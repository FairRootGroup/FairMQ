/********************************************************************************
 *   Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH     *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>
#include <FairMQLogger.h>

#include <iostream>
#include <stdexcept>

namespace fair
{
namespace mq
{
namespace test
{

class TestExceptions : public FairMQDevice
{
  public:
    auto Init() -> void override
    {
        std::string state("Init");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            throw std::runtime_error("exception in " + state + "()");
        }
    }

    auto InitTask() -> void override
    {
        std::string state("InitTask");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            throw std::runtime_error("exception in " + state + "()");
        }
    }

    auto PreRun() -> void override
    {
        std::string state("PreRun");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            throw std::runtime_error("exception in " + state + "()");
        }
    }

    auto Run() -> void override
    {
        std::string state("Run");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            throw std::runtime_error("exception in " + state + "()");
        }
    }

    auto PostRun() -> void override
    {
        std::string state("PostRun");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            throw std::runtime_error("exception in " + state + "()");
        }
    }

    auto ResetTask() -> void override
    {
        std::string state("ResetTask");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            throw std::runtime_error("exception in " + state + "()");
        }
    }

    auto Reset() -> void override
    {
        std::string state("Reset");
        if (std::string::npos != GetId().find("_" + state + "_")) {
            throw std::runtime_error("exception in " + state + "()");
        }
    }
};

} // namespace test
} // namespace mq
} // namespace fair
