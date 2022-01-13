/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_WAITFOR_H
#define FAIR_MQ_TEST_WAITFOR_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>

#include <iostream>

namespace fair::mq::test
{

class TestWaitFor : public FairMQDevice
{
  public:
    void PreRun() override
    {
        std::string state("PreRun");
        if (std::string::npos != GetId().find("_" + state)) {
            LOG(info) << "Going to sleep via WaitFor() in " << state << "...";
            bool result = WaitFor(std::chrono::seconds(60));
            LOG(info) << (result == true ? "Sleeping Done. Not interrupted." : "Sleeping Done. Interrupted.");
        }
    }

    void Run() override
    {
        std::string state("Run");
        if (std::string::npos != GetId().find("_" + state)) {
            LOG(info) << "Going to sleep via WaitFor() in " << state << "...";
            bool result = WaitFor(std::chrono::seconds(60));
            LOG(info) << (result == true ? "Sleeping Done. Not interrupted." : "Sleeping Done. Interrupted.");
        }
    }

    void PostRun() override
    {
        std::string state("PostRun");
        if (std::string::npos != GetId().find("_" + state)) {
            LOG(info) << "Going to sleep via WaitFor() in " << state << "...";
            bool result = WaitFor(std::chrono::seconds(60));
            LOG(info) << (result == true ? "Sleeping Done. Not interrupted." : "Sleeping Done. Interrupted.");
        }
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_WAITFOR_H */
