/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_TRANSPORTPARAM_H
#define FAIR_MQ_TEST_TRANSPORTPARAM_H

#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include <utility>

namespace fair::mq::test
{

/// The parameter of a test suite that runs once per transport.
/// The test names get the suffix Name(), e.g. "Resize.Basic/shmem".
struct TransportParam
{
    explicit TransportParam(std::string transportName, bool expanded = false)
        : transport(std::move(transportName))
        , expandedShmMetadata(expanded)
    {}

    std::string transport;
    bool expandedShmMetadata;

    auto Name() const -> std::string
    {
        return expandedShmMetadata ? transport + "_expanded_metadata" : transport;
    }

    /// The command line options for a device process (see RunOptions).
    auto DeviceArgs() const -> std::string
    {
        return expandedShmMetadata ? "--shm-metadata-msg-size 2048" : "";
    }
};

inline auto operator<<(std::ostream& os, TransportParam const& param) -> std::ostream&
{
    return os << param.Name();
}

/// zeromq, shmem, and shmem with the expanded metadata.
inline auto AllTransports()
{
    return testing::Values(TransportParam("zeromq"), TransportParam("shmem"), TransportParam("shmem", true));
}

/// shmem, and shmem with the expanded metadata.
inline auto ShmTransports()
{
    return testing::Values(TransportParam("shmem"), TransportParam("shmem", true));
}

/// zeromq and shmem.
inline auto BasicTransports()
{
    return testing::Values(TransportParam("zeromq"), TransportParam("shmem"));
}

/// Name generator for INSTANTIATE_TEST_SUITE_P().
struct TransportParamName
{
    auto operator()(testing::TestParamInfo<TransportParam> const& info) const -> std::string
    {
        return info.param.Name();
    }
};

using TransportTest = testing::TestWithParam<TransportParam>;

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_TRANSPORTPARAM_H */
