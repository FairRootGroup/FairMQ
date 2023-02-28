/********************************************************************************
 * Copyright (C) 2018-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_ERROR_STATE_H
#define FAIR_MQ_TEST_ERROR_STATE_H

#include <fairmq/Device.h>
#include <fairmq/Tools.h>

#include <fairlogger/Logger.h>

#include <string_view>

namespace fair::mq::test
{

class ErrorState : public Device
{
    auto ErrorIfRequestedById(std::string_view state)
    {
        if (std::string::npos != GetId().find(tools::ToString("_", state, "_"))) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeStateOrThrow(fair::mq::Transition::ErrorFound);
        }
    }

  public:
    void Init()      override { ErrorIfRequestedById("Init"); }
    void Bind()      override { ErrorIfRequestedById("Bind"); }
    void Connect()   override { ErrorIfRequestedById("Connect"); }
    void InitTask()  override { ErrorIfRequestedById("InitTask"); }
    void PreRun()    override { ErrorIfRequestedById("PreRun"); }
    void Run()       override { ErrorIfRequestedById("Run"); }
    void PostRun()   override { ErrorIfRequestedById("PostRun"); }
    void ResetTask() override { ErrorIfRequestedById("ResetTask"); }
    void Reset()     override { ErrorIfRequestedById("Reset"); }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_ERROR_STATE_H */
