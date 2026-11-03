/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_TRANSPORT_H
#define FAIR_MQ_TEST_TRANSPORT_H

#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Unique.h>

#include <cstddef>
#include <cstdio>   // std::remove
#include <string>
#include <string_view>

namespace fair::mq::test
{

constexpr std::size_t kShmSegmentSize = 100000000;

/// Set the properties that a transport factory in a test needs: the session,
/// the shm segment size, the shm monitor and (optionally) the expanded shm
/// metadata. Returns the session id. Set other properties after this call.
inline auto Configure(ProgOptions& config,
                      std::string session = tools::Uuid(),
                      bool expandedShmMetadata = false) -> std::string
{
    config.SetProperty<std::string>("session", session);
    config.SetProperty<std::size_t>("shm-segment-size", kShmSegmentSize);
    config.SetProperty<bool>("shm-monitor", true);
    if (expandedShmMetadata) {
        config.SetProperty<std::size_t>("shm-metadata-msg-size", 2048);
    }
    return session;
}

/// A unique ipc address. The socket file is in /tmp, and the destructor
/// removes it.
class IpcAddress
{
  public:
    explicit IpcAddress(std::string_view name)
        : fPath(std::string("/tmp/fmq_test_").append(name).append("_").append(std::to_string(tools::UuidHash())))
        , fAddress("ipc://" + fPath)
    {}

    IpcAddress(const IpcAddress&) = delete;
    IpcAddress(IpcAddress&&) = delete;
    IpcAddress& operator=(const IpcAddress&) = delete;
    IpcAddress& operator=(IpcAddress&&) = delete;

    ~IpcAddress() { std::remove(fPath.c_str()); }

    auto str() const -> std::string const& { return fAddress; }
    operator std::string const&() const { return fAddress; } // NOLINT(google-explicit-constructor)

  private:
    std::string fPath;
    std::string fAddress;
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_TRANSPORT_H */
