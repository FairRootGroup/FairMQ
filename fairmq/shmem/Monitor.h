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
#include <utility> // pair
#include <vector>

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

struct BufferDebugInfo
{
    BufferDebugInfo(size_t offset, pid_t pid, size_t size, uint64_t creationTime)
        : fOffset(offset)
        , fPid(pid)
        , fSize(size)
        , fCreationTime(creationTime)
    {}

    size_t fOffset;
    pid_t fPid;
    size_t fSize;
    uint64_t fCreationTime;
};

class Monitor
{
  public:
    Monitor(const std::string& shmId, bool selfDestruct, bool interactive, bool viewOnly, unsigned int timeoutInMS, unsigned int intervalInMS, bool runAsDaemon, bool cleanOnExit);

    Monitor(const Monitor&) = delete;
    Monitor operator=(const Monitor&) = delete;

    virtual ~Monitor();

    void CatchSignals();
    void Run();

    /// @brief Cleanup all shared memory artifacts created by devices
    /// @param shmId shared memory id
    /// @param verbose output cleanup results to stdout
    static std::vector<std::pair<std::string, bool>> Cleanup(const ShmId& shmId, bool verbose = true);
    /// @brief Cleanup all shared memory artifacts created by devices
    /// @param sessionId session id
    /// @param verbose output cleanup results to stdout
    static std::vector<std::pair<std::string, bool>> Cleanup(const SessionId& sessionId, bool verbose = true);
    /// @brief Cleanup all shared memory artifacts created by devices and monitors
    /// @param shmId shared memory id
    /// @param verbose output cleanup results to stdout
    static std::vector<std::pair<std::string, bool>> CleanupFull(const ShmId& shmId, bool verbose = true);
    /// @brief Cleanup all shared memory artifacts created by devices and monitors
    /// @param sessionId session id
    /// @param verbose output cleanup results to stdout
    static std::vector<std::pair<std::string, bool>> CleanupFull(const SessionId& sessionId, bool verbose = true);

    static void PrintDebugInfo(const ShmId& shmId);
    static void PrintDebugInfo(const SessionId& shmId);
    static std::unordered_map<uint16_t, std::vector<BufferDebugInfo>> GetDebugInfo(const ShmId& shmId);
    static std::unordered_map<uint16_t, std::vector<BufferDebugInfo>> GetDebugInfo(const SessionId& shmId);

    static bool RemoveObject(const std::string& name);
    static bool RemoveFileMapping(const std::string& name);
    static bool RemoveQueue(const std::string& name);
    static bool RemoveMutex(const std::string& name);
    static bool RemoveCondition(const std::string& name);

    struct DaemonPresent : std::runtime_error { using std::runtime_error::runtime_error; };

  private:
    void PrintHelp();
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
    unsigned int fIntervalInMS;
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
