/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSSAGENT_H
#define FAIR_MQ_SDK_DDSSAGENT_H

#include <fairmq/sdk/DDSSession.h>

#include <ostream>
#include <string>
#include <chrono>
#include <cstdint>

namespace fair::mq::sdk
{

/**
 * @class DDSAgent <fairmq/sdk/DDSAgent.h>
 * @brief Represents a DDS agent
 */
class DDSAgent
{
  public:
    using Id = uint64_t;
    using Pid = uint32_t;

    explicit DDSAgent(DDSSession session,
                      Id id,
                      Pid pid,
                      std::string path,
                      std::string host,
                      std::chrono::milliseconds startupTime,
                      std::string username)
        : fSession(std::move(session))
        , fId(id)
        , fPid(pid)
        , fDDSPath(std::move(path))
        , fHost(std::move(host))
        , fStartupTime(startupTime)
        , fUsername(std::move(username))
    {}

    DDSSession GetSession() const { return fSession; }
    Id GetId() const { return fId; }
    Pid GetPid() const { return fPid; }
    std::string GetHost() const { return fHost; }
    std::string GetDDSPath() const { return fDDSPath; }
    std::chrono::milliseconds GetStartupTime() const { return fStartupTime; }
    std::string GetUsername() const { return fUsername; }

    friend auto operator<<(std::ostream& os, const DDSAgent& agent) -> std::ostream&
    {
        return os << "DDSAgent id: " << agent.fId
                  << ", pid: " << agent.fPid
                  << ", path: " << agent.fDDSPath
                  << ", host: " << agent.fHost
                  << ", startupTime: " << agent.fStartupTime.count()
                  << ", username: " << agent.fUsername;
    }

  private:
    DDSSession fSession;
    Id fId;
    Pid fPid;
    std::string fDDSPath;
    std::string fHost;
    std::chrono::milliseconds fStartupTime;
    std::string fUsername;
};

} // namespace fair::mq::sdk

#endif /* FAIR_MQ_SDK_DDSSAGENT_H */
