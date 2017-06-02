/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_MONITOR_H_
#define FAIR_MQ_SHMEM_MONITOR_H_

#include <boost/interprocess/managed_shared_memory.hpp>

#include <thread>
#include <chrono>
#include <atomic>
#include <string>

namespace fair
{
namespace mq
{
namespace shmem
{

class Monitor
{
  public:
    Monitor(const std::string& segmentName, bool selfDestruct, bool interactive, unsigned int timeoutInMS);

    Monitor(const Monitor&) = delete;
    Monitor operator=(const Monitor&) = delete;

    void CatchSignals();
    void Run();

    virtual ~Monitor();

    static void Cleanup(const std::string& segmentName);
    static void CleanupControlQueues();

  private:
    void PrintHeader();
    void PrintHelp();
    void PrintQueues();
    void MonitorHeartbeats();
    void CheckSegment();
    void Interactive();
    void SignalMonitor();
    static void RemoveObject(const std::string&);

    bool fSelfDestruct; // will self-destruct after the memory has been closed
    bool fInteractive; // running in interactive mode
    bool fSeenOnce; // true is segment has been opened successfully at least once
    unsigned int fTimeoutInMS;
    std::string fSegmentName;
    std::atomic<bool> fTerminating;
    std::atomic<bool> fHeartbeatTriggered;
    std::chrono::high_resolution_clock::time_point fLastHeartbeat;
    std::thread fSignalThread;
    boost::interprocess::managed_shared_memory fManagementSegment;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_MONITOR_H_ */