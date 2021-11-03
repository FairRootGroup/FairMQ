/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_EXCEPTIONS_H
#define FAIR_MQ_TOOLS_EXCEPTIONS_H

#include <functional>

namespace fair::mq::tools
{

/**
 * Executes the given callback in the destructor.
 * Can be used to execute something in case of an exception when catch is undesirable, e.g.:
 *
 * {
 *    // callback will be executed only if f throws an exception
 *    CallOnDestruction cod([](){ cout << "exception was thrown"; }, true);
 *    f();
 *    cod.disable();
 * }
 */

class CallOnDestruction
{
  public:
    CallOnDestruction(std::function<void()> c, bool enable = true)
        : callback(c)
        , enabled(enable)
    {}

    ~CallOnDestruction()
    {
        if (enabled) {
            callback();
        }
    }

    void enable() { enabled = true; }
    void disable() { enabled = false; }

  private:
    std::function<void()> callback;
    bool enabled;
};

} // namespace fair::mq::tools

#endif /* FAIR_MQ_TOOLS_EXCEPTIONS_H */
