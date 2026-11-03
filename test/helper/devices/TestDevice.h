/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_TESTDEVICE_H
#define FAIR_MQ_TEST_TESTDEVICE_H

#include <fairmq/Device.h>

#include <fairlogger/Logger.h>

#include <stdexcept>
#include <string>
#include <string_view>

namespace fair::mq::test
{

/// Printed by a test device that has done all its checks. The test harness
/// expects it followed by the name of the transport that the device used.
constexpr std::string_view kSuccessMarker = "test device succeeded, transport: ";

/// Base class of the devices in runTestDevice.
class TestDevice : public Device
{
  protected:
    /// Report success; call it as the last step of the test.
    void Succeed() const { LOG(info) << kSuccessMarker << GetTransportName(); }

    /// Throw if the condition is false. The device then goes to the Error
    /// state and exits with a non-zero exit code.
    static void Check(bool condition, std::string_view what)
    {
        if (!condition) {
            throw std::runtime_error(std::string("check failed: ").append(what));
        }
    }

    /// The content of a message as a string.
    static auto AsString(Message const& msg) -> std::string
    {
        return {static_cast<char const*>(msg.GetData()), msg.GetSize()};
    }
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_TESTDEVICE_H */
