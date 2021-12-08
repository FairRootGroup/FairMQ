/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include <fairmq/shmem/Common.h>
#include <fairmq/shmem/UnmanagedRegion.h>
#include <fairmq/shmem/Segment.h>
#include <fairmq/shmem/Monitor.h>

#include <fairmq/tools/Unique.h>

#include <fairlogger/Logger.h>

#include <csignal>

#include <chrono>
#include <string>
#include <thread>

using namespace std;

namespace
{
    volatile sig_atomic_t gStopping = 0;
}

void signalHandler(int /* signal */)
{
    gStopping = 1;
}

struct ShmRemover
{
    ShmRemover(std::string _shmId) : shmId(std::move(_shmId)) {}
    ~ShmRemover()
    {
        // This will clean all segments, regions and any other shmem objects belonging to this shmId
        fair::mq::shmem::Monitor::Cleanup(fair::mq::shmem::ShmId{shmId});
    }

    std::string shmId;
};

int main(int /* argc */, char** /* argv */)
{
    fair::Logger::SetConsoleColor(true);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        const string session = "default"; // to_string(fair::mq::tools::UuidHash());
        // generate shmId out of session id + user id (geteuid).
        const string shmId = fair::mq::shmem::makeShmIdStr(session);

        const uint16_t s1id = 0;
        const uint64_t s1size = 100000000;
        const uint16_t s2id = 1;
        const uint64_t s2size = 200000000;

        const uint16_t r1id = 0;
        const uint64_t r1size = 100000000;
        const uint16_t r2id = 1;
        const uint64_t r2size = 200000000;

        // cleanup when done
        ShmRemover shmRemover(shmId);

        // managed segments
        fair::mq::shmem::Segment segment1(shmId, s1id, s1size, fair::mq::shmem::rbTreeBestFit);
        segment1.Lock();
        segment1.Zero();
        LOG(info) << "Created segment " << s1id << " of size " << segment1.GetSize() << " starting at " << segment1.GetData();

        fair::mq::shmem::Segment segment2(shmId, s2id, s2size, fair::mq::shmem::rbTreeBestFit);
        segment2.Lock();
        segment2.Zero();
        LOG(info) << "Created segment " << s2id << " of size " << segment2.GetSize() << " starting at " << segment2.GetData();

        // unmanaged regions
        fair::mq::shmem::UnmanagedRegion region1(shmId, r1id, r1size);
        region1.Lock();
        region1.Zero();
        LOG(info) << "Created region " << r1id << " of size " << region1.GetSize() << " starting at " << region1.GetData();

        fair::mq::shmem::UnmanagedRegion region2(shmId, r2id, r2size);
        region2.Lock();
        region2.Zero();
        LOG(info) << "Created region " << r2id << " of size " << region2.GetSize() << " starting at " << region2.GetData();

        // for a "soft reset" call (shmem should not be in active use by (no messages in flight) devices during this call):
        // fair::mq::shmem::Monitor::ResetContent(fair::mq::shmem::ShmId{shmId});

        while (!gStopping) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        LOG(info) << "stopping.";
    } catch (exception& e) {
        LOG(error) << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit";
        return 2;
    }

    return 0;
}
