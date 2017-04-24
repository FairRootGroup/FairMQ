/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQSHMMONITOR_H_
#define FAIRMQSHMMONITOR_H_

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
    Monitor(const std::string& segmentName);
    Monitor(const Monitor&) = delete;
    Monitor operator=(const Monitor&) = delete;

    void Run();

    virtual ~Monitor() {}

  private:
    void PrintHeader();
    void PrintHelp();
    void PrintQueues();
    void CloseMemory();
    void MonitorHeartbeats();
    void CheckSegment();

    static void Cleanup(const std::string& segmentName);

    std::string fSegmentName;
    std::atomic<bool> fTerminating;
    std::atomic<bool> fHeartbeatTriggered;
    std::chrono::high_resolution_clock::time_point fLastHeartbeat;
    std::thread fHeartbeatThread;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIRMQSHMMONITOR_H_ */