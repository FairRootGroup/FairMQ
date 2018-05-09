/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/shmem/Monitor.h>
#include <fairmq/shmem/Common.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <csignal>
#include <iostream>
#include <iomanip>

#include <termios.h>
#include <poll.h>

using namespace std;
namespace bipc = boost::interprocess;
namespace bpt = boost::posix_time;

using CharAllocator   = bipc::allocator<char, bipc::managed_shared_memory::segment_manager>;
using String          = bipc::basic_string<char, char_traits<char>, CharAllocator>;
using StringAllocator = bipc::allocator<String, bipc::managed_shared_memory::segment_manager>;
using StringVector    = bipc::vector<String, StringAllocator>;

namespace
{
    volatile sig_atomic_t gSignalStatus = 0;
}

namespace fair
{
namespace mq
{
namespace shmem
{

void signalHandler(int signal)
{
    gSignalStatus = signal;
}

Monitor::Monitor(const string& sessionName, bool selfDestruct, bool interactive, unsigned int timeoutInMS, bool runAsDaemon, bool cleanOnExit)
    : fSelfDestruct(selfDestruct)
    , fInteractive(interactive)
    , fSeenOnce(false)
    , fIsDaemon(runAsDaemon)
    , fCleanOnExit(cleanOnExit)
    , fTimeoutInMS(timeoutInMS)
    , fSessionName(sessionName)
    , fSegmentName("fmq_" + fSessionName + "_main")
    , fManagementSegmentName("fmq_" + fSessionName + "_mng")
    , fControlQueueName("fmq_" + fSessionName + "_cq")
    , fTerminating(false)
    , fHeartbeatTriggered(false)
    , fLastHeartbeat(chrono::high_resolution_clock::now())
    , fSignalThread()
    , fManagementSegment(bipc::open_or_create, fManagementSegmentName.c_str(), 65536)
    , fDeviceHeartbeats()
{
    MonitorStatus* monitorStatus = fManagementSegment.find<MonitorStatus>(bipc::unique_instance).first;
    if (monitorStatus != nullptr)
    {
        cout << "fairmq-shmmonitor already started or not properly exited. Try `fairmq-shmmonitor --cleanup`" << endl;
        exit(EXIT_FAILURE);
    }
    fManagementSegment.construct<MonitorStatus>(bipc::unique_instance)();

    RemoveQueue(fControlQueueName);
}

void Monitor::CatchSignals()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    fSignalThread = thread(&Monitor::SignalMonitor, this);
}

void Monitor::SignalMonitor()
{
    while (true)
    {
        if (gSignalStatus != 0)
        {
            fTerminating = true;
            cout << "signal: " << gSignalStatus << endl;
            break;
        }
        else if (fTerminating)
        {
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void Monitor::Run()
{
    thread heartbeatThread(&Monitor::MonitorHeartbeats, this);

    if (fInteractive)
    {
        Interactive();
    }
    else
    {
        while (!fTerminating)
        {
            this_thread::sleep_for(chrono::milliseconds(100));
            CheckSegment();
        }
    }

    heartbeatThread.join();
}

void Monitor::MonitorHeartbeats()
{
    try
    {
        bipc::message_queue mq(bipc::open_or_create, fControlQueueName.c_str(), 1000, 256);

        unsigned int priority;
        bipc::message_queue::size_type recvdSize;
        char msg[256] = {0};

        while (!fTerminating)
        {
            bpt::ptime rcvTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(100);
            if (mq.timed_receive(&msg, sizeof(msg), recvdSize, priority, rcvTill))
            {
                fHeartbeatTriggered = true;
                fLastHeartbeat = chrono::high_resolution_clock::now();
                string deviceId(msg, recvdSize);
                fDeviceHeartbeats[deviceId] = fLastHeartbeat;
            }
            else
            {
                // cout << "control queue timeout" << endl;
            }
        }
    }
    catch (bipc::interprocess_exception& ie)
    {
        cout << ie.what() << endl;
    }

    RemoveQueue(fControlQueueName);
}

void Monitor::Interactive()
{
    char c;
    pollfd cinfd[1];
    cinfd[0].fd = fileno(stdin);
    cinfd[0].events = POLLIN;

    struct termios t;
    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag &= ~ICANON; // disable canonical input
    t.c_lflag &= ~ECHO; // do not echo input chars
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

    cout << endl;
    PrintHelp();
    cout << endl;
    PrintHeader();

    while (!fTerminating)
    {
        if (poll(cinfd, 1, 100))
        {
            if (fTerminating || gSignalStatus != 0)
            {
                break;
            }

            c = getchar();

            switch (c)
            {
                case 'q':
                    cout << "\n[q] --> quitting." << endl;
                    fTerminating = true;
                    break;
                case 'p':
                    cout << "\n[p] --> active queues:" << endl;
                    PrintQueues();
                    break;
                case 'x':
                    cout << "\n[x] --> closing shared memory:" << endl;
                    Cleanup(fSessionName);
                    break;
                case 'h':
                    cout << "\n[h] --> help:" << endl << endl;
                    PrintHelp();
                    cout << endl;
                    break;
                case '\n':
                    cout << "\n[\\n] --> invalid input." << endl;
                    break;
                default:
                    cout << "\n[" << c << "] --> invalid input." << endl;
                    break;
            }

            if (fTerminating)
            {
                break;
            }

            PrintHeader();
        }

        if (fTerminating)
        {
            break;
        }

        CheckSegment();

        if (!fTerminating)
        {
            cout << "\r";
        }
    }

    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag |= ICANON; // re-enable canonical input
    t.c_lflag |= ECHO; // echo input chars
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
}

void Monitor::CheckSegment()
{
    static uint64_t counter = 0;
    char c = '#';

    if (fInteractive)
    {
        int mod = counter++ % 5;
        switch (mod)
        {
            case 0:
                c = '-';
                break;
            case 1:
                c = '\\';
                break;
            case 2:
                c = '|';
                break;
            case 3:
                c = '-';
                break;
            case 4:
                c = '/';
                break;
            default:
                break;
        }
    }

    try
    {
        bipc::managed_shared_memory segment(bipc::open_only, fSegmentName.c_str());

        fSeenOnce = true;

        unsigned int numDevices = 0;

        fair::mq::shmem::DeviceCounter* dc = segment.find<fair::mq::shmem::DeviceCounter>(bipc::unique_instance).first;
        if (dc)
        {
            numDevices = dc->fCount;
        }

        auto now = chrono::high_resolution_clock::now();
        unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat).count();

        if (fHeartbeatTriggered && duration > fTimeoutInMS)
        {
            cout << "no heartbeats since over " << fTimeoutInMS << " milliseconds, cleaning..." << endl;
            Cleanup(fSessionName);
            fHeartbeatTriggered = false;
            if (fSelfDestruct)
            {
                cout << "\nself destructing" << endl;
                fTerminating = true;
            }
        }

        if (fInteractive)
        {
            cout << "| "
                << setw(18) << fSegmentName << " | "
                << setw(10) << segment.get_size() << " | "
                << setw(10) << segment.get_free_memory() << " | "
                // << setw(15) << segment.all_memory_deallocated() << " | "
                << setw(2) << segment.check_sanity() << " | "
                // << setw(10) << segment.get_num_named_objects() << " | "
                << setw(10) << numDevices << " | "
                // << setw(10) << segment.get_num_unique_objects() << " |"
                << setw(10) << duration << " |"
                << c
                << flush;
        }
    }
    catch (bipc::interprocess_exception& ie)
    {
        fHeartbeatTriggered = false;
        if (fInteractive)
        {
            cout << "| "
                << setw(18) << "-" << " | "
                << setw(10) << "-" << " | "
                << setw(10) << "-" << " | "
                // << setw(15) << "-" << " | "
                << setw(2) << "-" << " | "
                << setw(10) << "-" << " | "
                << setw(10) << "-" << " |"
                << c
                << flush;
        }

        auto now = chrono::high_resolution_clock::now();
        unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat).count();

        if (fIsDaemon && duration > fTimeoutInMS * 2)
        {
            Cleanup(fSessionName);
            fHeartbeatTriggered = false;
            if (fSelfDestruct)
            {
                cout << "\nself destructing" << endl;
                fTerminating = true;
            }
        }

        if (fSelfDestruct)
        {
            if (fSeenOnce)
            {
                cout << "self destructing" << endl;
                fTerminating = true;
            }
        }
    }
}

void Monitor::Cleanup(const string& sessionName)
{
    string managementSegmentName("fmq_" + sessionName + "_mng");
    try
    {
        bipc::managed_shared_memory managementSegment(bipc::open_only, managementSegmentName.c_str());
        RegionCounter* rc = managementSegment.find<RegionCounter>(bipc::unique_instance).first;
        if (rc)
        {
            cout << "Region counter found: " << rc->fCount << endl;
            unsigned int regionCount = rc->fCount;
            for (unsigned int i = 1; i <= regionCount; ++i)
            {
                RemoveObject("fmq_" + sessionName + "_rg_" + to_string(i));
                RemoveQueue(string("fmq_" + sessionName + "_rgq_" + to_string(i)));
            }
        }
        else
        {
            cout << "shmem: no region counter found. no regions to cleanup." << endl;
        }

        RemoveObject(managementSegmentName.c_str());
    }
    catch (bipc::interprocess_exception& ie)
    {
        cout << "Did not find '" << managementSegmentName << "' shared memory segment. No regions to cleanup." << endl;
    }

    RemoveObject("fmq_" + sessionName + "_main");

    boost::interprocess::named_mutex::remove(string("fmq_" + sessionName + "_mtx").c_str());

    cout << endl;
}

void Monitor::RemoveObject(const string& name)
{
    if (bipc::shared_memory_object::remove(name.c_str()))
    {
        cout << "Successfully removed \"" << name << "\"." << endl;
    }
    else
    {
        cout << "Did not remove \"" << name << "\". Already removed?" << endl;
    }
}

void Monitor::RemoveQueue(const string& name)
{
    if (bipc::message_queue::remove(name.c_str()))
    {
        cout << "Successfully removed \"" << name << "\"." << endl;
    }
    else
    {
        cout << "Did not remove \"" << name << "\". Already removed?" << endl;
    }
}

void Monitor::PrintQueues()
{
    cout << '\n';

    try
    {
        bipc::managed_shared_memory segment(bipc::open_only, fSegmentName.c_str());
        StringVector* queues = segment.find<StringVector>(string("fmq_" + fSessionName + "_qs").c_str()).first;
        if (queues)
        {
            cout << "found " << queues->size() << " queue(s):" << endl;

            for (unsigned int i = 0; i < queues->size(); ++i)
            {
                string name(queues->at(i).c_str());
                cout << '\t' << name << " : ";
                atomic<int>* queueSize = segment.find<atomic<int>>(name.c_str()).first;
                if (queueSize)
                {
                    cout << *queueSize << " messages" << endl;
                }
                else
                {
                    cout << "\tqueue does not have a queue size entry." << endl;
                }
            }
        }
        else
        {
            cout << "\tno queues found" << endl;
        }
    }
    catch (bipc::interprocess_exception& ie)
    {
        cout << "\tno queues found" << endl;
    }
    catch (out_of_range& ie)
    {
        cout << "\tno queues found" << endl;
    }

    cout << "\n    --> last heartbeats: " << endl << endl;
    auto now = chrono::high_resolution_clock::now();
    for (const auto& h : fDeviceHeartbeats)
    {
        cout << "\t" << h.first << " : " << chrono::duration<double, milli>(now - h.second).count() << "ms ago." << endl;
    }

    cout << endl;
}

void Monitor::PrintHeader()
{
    cout << "| "
        << "\033[01;32m" << setw(18) << "name"            << "\033[0m" << " | "
        << "\033[01;32m" << setw(10) << "size"            << "\033[0m" << " | "
        << "\033[01;32m" << setw(10) << "free"            << "\033[0m" << " | "
        // << "\033[01;32m" << setw(15) << "all deallocated" << "\033[0m" << " | "
        << "\033[01;32m" << setw(2)  << "ok"              << "\033[0m" << " | "
        // << "\033[01;32m" << setw(10) << "# named"         << "\033[0m" << " | "
        << "\033[01;32m" << setw(10) << "# devices"       << "\033[0m" << " | "
        // << "\033[01;32m" << setw(10) << "# unique"        << "\033[0m" << " |"
        << "\033[01;32m" << setw(10) << "ms since"        << "\033[0m" << " |"
        << endl;
}

void Monitor::PrintHelp()
{
    cout << "controls: [x] close memory, [p] print queues, [h] help, [q] quit." << endl;
}

Monitor::~Monitor()
{
    fManagementSegment.destroy<MonitorStatus>(bipc::unique_instance);
    if (fSignalThread.joinable())
    {
        fSignalThread.join();
    }
    if (fCleanOnExit)
    {
        Cleanup(fSessionName);
    }
}

} // namespace shmem
} // namespace mq
} // namespace fair
