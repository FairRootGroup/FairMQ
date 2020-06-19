/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_MONITOR_H_
#define FAIR_MQ_SHMEM_MONITOR_H_

#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <stdexcept>
#include <unordered_map>

namespace fair
{
namespace mq
{
namespace shmem
{

struct SessionId
{
    std::string sessionId;
    explicit operator std::string() const { return sessionId; }
};

struct ShmId
{
    std::string shmId;
    explicit operator std::string() const { return shmId; }
};

class Monitor
{
  public:
    Monitor(const std::string& shmId, bool selfDestruct, bool interactive, bool viewOnly, unsigned int timeoutInMS, bool runAsDaemon, bool cleanOnExit);

    Monitor(const Monitor&) = delete;
    Monitor operator=(const Monitor&) = delete;

    virtual ~Monitor();

    void CatchSignals();
    void Run();

    /// @brief Cleanup all shared memory artifacts created by devices
    /// @param shmId shared memory id
    static void Cleanup(const ShmId& shmId);
    /// @brief Cleanup all shared memory artifacts created by devices
    /// @param sessionId session id
    static void Cleanup(const SessionId& sessionId);
    /// @brief Cleanup all shared memory artifacts created by devices and monitors
    /// @param shmId shared memory id
    static void CleanupFull(const ShmId& shmId);
    /// @brief Cleanup all shared memory artifacts created by devices and monitors
    /// @param sessionId session id
    static void CleanupFull(const SessionId& sessionId);

    static void RemoveObject(const std::string&);
    static void RemoveFileMapping(const std::string&);
    static void RemoveQueue(const std::string&);
    static void RemoveMutex(const std::string&);
    static void RemoveCondition(const std::string&);

    struct DaemonPresent : std::runtime_error { using std::runtime_error::runtime_error; };

  private:
    void PrintHeader();
    void PrintHelp();
    void PrintQueues();
    void MonitorHeartbeats();
    void CheckSegment();
    void Interactive();
    void SignalMonitor();

    bool fSelfDestruct; // will self-destruct after the memory has been closed
    bool fInteractive; // running in interactive mode
    bool fViewOnly; // view only mode
    bool fIsDaemon;
    bool fSeenOnce; // true is segment has been opened successfully at least once
    bool fCleanOnExit;
    unsigned int fTimeoutInMS;
    std::string fShmId;
    std::string fSegmentName;
    std::string fManagementSegmentName;
    std::string fControlQueueName;
    std::atomic<bool> fTerminating;
    std::atomic<bool> fHeartbeatTriggered;
    std::chrono::high_resolution_clock::time_point fLastHeartbeat;
    std::thread fSignalThread;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> fDeviceHeartbeats;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_MONITOR_H_ */
