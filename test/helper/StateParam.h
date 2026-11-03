/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_STATEPARAM_H
#define FAIR_MQ_TEST_STATEPARAM_H

#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace fair::mq::test
{

/// The parameter of a device test that runs an action (see StateAction) in
/// one state, with one control mode. The test names get the suffix Name(),
/// e.g. "Exceptions.Throw/interactive_InitTask".
struct StateParam
{
    StateParam(std::string stateName, std::string controlName)
        : state(std::move(stateName))
        , control(std::move(controlName))
    {}

    std::string state;
    std::string control;

    auto Name() const -> std::string { return control + "_" + state; }

    /// The stdin input for the device. With interactive control, a device
    /// that fails in ResetTask or Reset waits for "q" to quit.
    auto Input() const -> std::string
    {
        return (control == "interactive" && (state == "ResetTask" || state == "Reset")) ? "q" : "";
    }
};

inline auto operator<<(std::ostream& os, StateParam const& param) -> std::ostream&
{
    return os << param.Name();
}

/// The device states with a user hook.
inline auto HookStates() -> std::vector<std::string>
{
    return {"Init", "Bind", "Connect", "InitTask", "PreRun", "Run", "PostRun", "ResetTask", "Reset"};
}

/// Each state with the static and with the interactive control.
inline auto WithControls(std::vector<std::string> const& states)
{
    std::vector<StateParam> params;
    for (auto const& control : {"static", "interactive"}) {
        for (auto const& state : states) {
            params.emplace_back(state, control);
        }
    }
    return testing::ValuesIn(params);
}

/// Name generator for INSTANTIATE_TEST_SUITE_P().
struct StateParamName
{
    auto operator()(testing::TestParamInfo<StateParam> const& info) const -> std::string
    {
        return info.param.Name();
    }
};

using StateTest = testing::TestWithParam<StateParam>;

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_STATEPARAM_H */
