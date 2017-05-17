/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQShmMonitor.h"
#include "FairMQShmDeviceCounter.h"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <iomanip>

#include <termios.h>
#include <poll.h>

using namespace std;
namespace bipc = boost::interprocess;
namespace bpt = boost::posix_time;

using CharAllocator   = bipc::allocator<char, bipc::managed_shared_memory::segment_manager>;
using String          = bipc::basic_string<char, std::char_traits<char>, CharAllocator>;
using StringAllocator = bipc::allocator<String, bipc::managed_shared_memory::segment_manager>;
using StringVector    = bipc::vector<String, StringAllocator>;

namespace fair
{
namespace mq
{
namespace shmem
{

Monitor::Monitor(const string& segmentName)
    : fSegmentName(segmentName)
    , fTerminating(false)
    , fHeartbeatTriggered(false)
    , fLastHeartbeat()
    , fHeartbeatThread()
{
    if (bipc::message_queue::remove("fairmq_shmem_control_queue"))
    {
        // cout << "successfully removed control queue" << endl;
    }
    else
    {
        // cout << "could not remove control queue" << endl;
    }
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

void Monitor::CloseMemory()
{
    if (bipc::shared_memory_object::remove(fSegmentName.c_str()))
    {
        cout << "Successfully removed shared memory \"" << fSegmentName.c_str() << "\"." << endl;
    }
    else
    {
        cout << "Did not remove shared memory. Already removed?" << endl;
    }
}

void Monitor::MonitorHeartbeats()
{
    try
    {
        bipc::message_queue mq(bipc::open_or_create, "fairmq_shmem_control_queue", 1000, sizeof(bool));

        unsigned int priority;
        bipc::message_queue::size_type recvdSize;

        while (!fTerminating)
        {
            bool heartbeat;
            bpt::ptime rcvTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(100);
            if (mq.timed_receive(&heartbeat, sizeof(heartbeat), recvdSize, priority, rcvTill))
            {
                fHeartbeatTriggered = true;
                fLastHeartbeat = chrono::high_resolution_clock::now();
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

    if (bipc::message_queue::remove("fairmq_shmem_control_queue"))
    {
        cout << "successfully removed control queue" << endl;
    }
    else
    {
        cout << "could not remove control queue" << endl;
    }
}

void Monitor::Run()
{
    thread heartbeatThread(&Monitor::MonitorHeartbeats, this);

    char input;
    pollfd inputFd[1];
    inputFd[0].fd = fileno(stdin);
    inputFd[0].events = POLLIN;

    struct termios t;
    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag &= ~ICANON; // disable canonical input
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

    cout << endl;
    PrintHelp();
    cout << endl;
    PrintHeader();

    while (!fTerminating)
    {
        if (poll(inputFd, 1, 100))
        {
            input = getchar();

            switch (input)
            {
                case 'q':
                    cout << "[q] --> quitting." << endl;
                    fTerminating = true;
                    break;
                case 'p':
                    cout << "[p] --> active queues:" << endl;
                    PrintQueues();
                    break;
                case 'x':
                    cout << "[x] --> closing shared memory:" << endl;
                    CloseMemory();
                    break;
                case 'h':
                    cout << "[h] --> help:" << endl << endl;
                    PrintHelp();
                    cout << endl;
                    break;
                case '\n':
                    cout << "[\\n] --> invalid input." << endl;
                    break;
                default:
                    cout << "[" << input << "] --> invalid input." << endl;
                    break;
            }

            if (fTerminating)
            {
                break;
            }

            PrintHeader();
        }

        CheckSegment();

        cout << "\r";
    }

    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag |= ICANON; // re-enable canonical input
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

    heartbeatThread.join();
}

void Monitor::CheckSegment()
{
    static uint64_t counter = 0;

    char c = '#';
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

    try
    {
        bipc::managed_shared_memory segment(bipc::open_only, fSegmentName.c_str());

        unsigned int numDevices = 0;

        pair<fair::mq::shmem::DeviceCounter*, size_t> result = segment.find<fair::mq::shmem::DeviceCounter>(bipc::unique_instance);
        if (result.first != nullptr)
        {
            numDevices = result.first->count;
        }

        auto now = chrono::high_resolution_clock::now();
        unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat).count();

        if (fHeartbeatTriggered && duration > 5000)
        {
            cout << "no heartbeats since over 5 seconds, cleaning..." << endl;
            CloseMemory();
            fHeartbeatTriggered = false;
        }

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
    catch (bipc::interprocess_exception& ie)
    {
        fHeartbeatTriggered = false;
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
}

void Monitor::Cleanup(const string& segmentName)
{
    if (bipc::shared_memory_object::remove(segmentName.c_str()))
    {
        cout << "Successfully removed shared memory \"" << segmentName.c_str() << "\"." << endl;
    }
    else
    {
        cout << "Did not remove shared memory. Already removed?" << endl;
    }
}

void Monitor::PrintQueues()
{
    cout << '\n';

    try
    {
        bipc::managed_shared_memory segment(bipc::open_only, fSegmentName.c_str());
        pair<StringVector*, size_t> queues = segment.find<StringVector>("fairmq_shmem_queues");
        if (queues.first != nullptr)
        {
            cout << "found " << queues.first->size() << " queue(s):" << endl;

            for (int i = 0; i < queues.first->size(); ++i)
            {
                string name(queues.first->at(i).c_str());
                cout << '\t' << name << " : ";
                pair<atomic<int>*, size_t> queueSize = segment.find<atomic<int>>(name.c_str());
                if (queueSize.first != nullptr)
                {
                    cout << *(queueSize.first) << " messages" << endl;
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
    catch (std::out_of_range& ie)
    {
        cout << "\tno queues found" << endl;
    }

    cout << endl;
}

} // namespace shmem
} // namespace mq
} // namespace fair
