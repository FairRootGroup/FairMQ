/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_MONITOR_H_
#define FAIR_MQ_SHMEM_MONITOR_H_

#include <fairlogger/Logger.h>

#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <utility> // pair
#include <vector>

namespace fair::mq::shmem
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
    Monitor(std::string shmId, bool selfDestruct, bool interactive, bool viewOnly, unsigned int timeoutInMS, unsigned int intervalInMS, bool runAsDaemon, bool cleanOnExit);

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

    /// @brief Outputs list of messages in shmem (if compiled with FAIRMQ_DEBUG_MODE=ON)
    /// @param shmId shmem id
    static void PrintDebugInfo(const ShmId& shmId);
    /// @brief Outputs list of messages in shmem (if compiled with FAIRMQ_DEBUG_MODE=ON)
    /// @param sessionId session id
    static void PrintDebugInfo(const SessionId& sessionId);
    /// @brief Returns a list of messages in shmem (if compiled with FAIRMQ_DEBUG_MODE=ON)
    /// @param shmId shmem id
    static std::unordered_map<uint16_t, std::vector<BufferDebugInfo>> GetDebugInfo(const ShmId& shmId);
    /// @brief Returns a list of messages in shmem (if compiled with FAIRMQ_DEBUG_MODE=ON)
    /// @param sessionId session id
    static std::unordered_map<uint16_t, std::vector<BufferDebugInfo>> GetDebugInfo(const SessionId& sessionId);
    /// @brief Returns the amount of free memory in the specified segment
    /// @param sessionId shmem id
    /// @param segmentId segment id
    /// @throws MonitorError
    static unsigned long GetFreeMemory(const ShmId& shmId, uint16_t segmentId);
    /// @brief Returns the amount of free memory in the specified segment
    /// @param sessionId session id
    /// @param segmentId segment id
    /// @throws MonitorError
    static unsigned long GetFreeMemory(const SessionId& sessionId, uint16_t segmentId);

    static bool PrintShm(const ShmId& shmId);
    static void ListAll(const std::string& path);

    static bool RemoveObject(const std::string& name);
    static bool RemoveFileMapping(const std::string& name);
    static bool RemoveQueue(const std::string& name);
    static bool RemoveMutex(const std::string& name);
    static bool RemoveCondition(const std::string& name);

    struct DaemonPresent : std::runtime_error { using std::runtime_error::runtime_error; };
    struct MonitorError : std::runtime_error { using std::runtime_error::runtime_error; };

  private:
    void PrintHelp();
    void Watch();
    void ReceiveHeartbeats();
    void CheckSegment();
    void Interactive();
    void SignalMonitor();

    bool fSelfDestruct; // will self-destruct after the memory has been closed
    bool fInteractive; // running in interactive mode
    bool fViewOnly; // view only mode
    bool fMonitor;
    bool fSeenOnce; // true is segment has been opened successfully at least once
    bool fCleanOnExit;
    unsigned int fTimeoutInMS;
    unsigned int fIntervalInMS;
    std::string fShmId;
    std::string fControlQueueName;
    std::atomic<bool> fTerminating;
    std::atomic<bool> fHeartbeatTriggered;
    std::chrono::high_resolution_clock::time_point fLastHeartbeat;
    std::thread fSignalThread;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> fDeviceHeartbeats;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_MONITOR_H_ */
