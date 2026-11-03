/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_STATEACTION_H
#define FAIR_MQ_TEST_STATEACTION_H

#include <fairmq/Device.h>

#include <fairlogger/Logger.h>

#include <chrono>
#include <csignal>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fair::mq::test
{

/// Does one action in one state handler. The device id selects both:
/// "<action>_<State>_", e.g. "exceptions_InitTask_". Actions:
/// - exceptions:  throw an exception
/// - error_state: change to the Error state
/// - signals:     raise SIGINT
/// - waitfor:     wait 60 s in WaitFor(); the test interrupts the wait
class StateAction : public Device
{
  public:
    void Init() override { ActIfRequested("Init"); }
    void Bind() override { ActIfRequested("Bind"); }
    void Connect() override { ActIfRequested("Connect"); }
    void InitTask() override { ActIfRequested("InitTask"); }
    void PreRun() override { ActIfRequested("PreRun"); }
    void Run() override { ActIfRequested("Run"); }
    void PostRun() override { ActIfRequested("PostRun"); }
    void ResetTask() override { ActIfRequested("ResetTask"); }
    void Reset() override { ActIfRequested("Reset"); }

  private:
    void ActIfRequested(std::string_view state)
    {
        auto const id = GetId();
        if (std::string::npos == id.find(std::string("_").append(state).append("_"))) {
            return;
        }

        if (id.rfind("exceptions_", 0) == 0) {
            throw std::runtime_error(std::string("exception in ").append(state).append("()"));
        } else if (id.rfind("error_state_", 0) == 0) {
            LOG(info) << "going to change to Error state from " << state << "()";
            ChangeStateOrThrow(Transition::ErrorFound);
        } else if (id.rfind("signals_", 0) == 0) {
            LOG(info) << "raising SIGINT from " << state << "()";
            std::raise(SIGINT);
        } else if (id.rfind("waitfor_", 0) == 0) {
            LOG(info) << "Going to sleep via WaitFor() in " << state << "...";
            bool const done = WaitFor(std::chrono::seconds(60));
            LOG(info) << (done ? "Sleeping Done. Not interrupted." : "Sleeping Done. Interrupted.");
        } else {
            throw std::runtime_error("unknown action in device id '" + id + "'");
        }
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_STATEACTION_H */
