/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQSTATES_H_
#define FAIRMQSTATES_H_

#include <string>
#include <ostream>
#include <stdexcept>

namespace fair
{
namespace mq
{

enum class State : int
{
    Undefined = 0,
    Ok,
    Error,
    Idle,
    InitializingDevice,
    Initialized,
    Binding,
    Bound,
    Connecting,
    DeviceReady,
    InitializingTask,
    Ready,
    Running,
    ResettingTask,
    ResettingDevice,
    Exiting
};

enum class Transition : int
{
    Auto = 0,
    InitDevice,
    CompleteInit,
    Bind,
    Connect,
    InitTask,
    Run,
    Stop,
    ResetTask,
    ResetDevice,
    End,
    ErrorFound
};

std::string GetStateName(State);
std::string GetTransitionName(Transition);
State GetState(const std::string& state);
Transition GetTransition(const std::string& transition);

struct DeviceErrorState : std::runtime_error { using std::runtime_error::runtime_error; };

inline std::ostream& operator<<(std::ostream& os, const State& state) { return os << GetStateName(state); }
inline std::ostream& operator<<(std::ostream& os, const Transition& transition) { return os << GetTransitionName(transition); }

} // namespace mq
} // namespace fair

#endif /* FAIRMQSTATES_H_ */
