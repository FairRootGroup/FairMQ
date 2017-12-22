/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQShmPrototypeSink.cxx
 *
 * @since 2016-04-08
 * @author A. Rybalchenko
 */

#include <string>
#include <thread>
#include <chrono>
#include <iomanip>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>

#include "FairMQShmPrototypeSink.h"
#include "FairMQProgOptions.h"
#include "FairMQLogger.h"

#include "ShmChunk.h"

using namespace std;
using namespace boost::interprocess;

FairMQShmPrototypeSink::FairMQShmPrototypeSink()
    : fBytesIn(0)
    , fMsgIn(0)
    , fBytesInNew(0)
    , fMsgInNew(0)
{
}

FairMQShmPrototypeSink::~FairMQShmPrototypeSink()
{
}

void FairMQShmPrototypeSink::Init()
{
    SegmentManager::Instance().InitializeSegment("open_or_create", "FairMQSharedMemoryPrototype", 2000000000);
    LOG(info) << "Created/Opened shared memory segment of 2,000,000,000 bytes. Available are "
              << SegmentManager::Instance().Segment()->get_free_memory() << " bytes.";
}

void FairMQShmPrototypeSink::Run()
{
    static uint64_t numReceivedMsgs = 0;

    thread rateLogger(&FairMQShmPrototypeSink::Log, this, 1000);

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessagePtr msg(NewMessage());

        if (Receive(msg, "meta") > 0)
        {
            ExMetaHeader* hdr = static_cast<ExMetaHeader*>(msg->GetData());
            size_t size = hdr->fSize;
            bipc::managed_shared_memory::handle_t handle = hdr->fHandle;
            void* ptr = SegmentManager::Instance().Segment()->get_address_from_handle(handle);

            // LOG(info) << size;
            // LOG(info) << handle;
            // LOG(warn) << ptr;

            fBytesInNew += size;
            ++fMsgInNew;
            SegmentManager::Instance().Segment()->deallocate(ptr);

            // get the shared pointer ID from the received message
            // string ownerID(static_cast<char*>(msg->GetData()), msg->GetSize());

            // find the shared pointer in shared memory with its ID
            // ShPtrOwner* owner = SegmentManager::Instance().Segment()->find<ShPtrOwner>(ownerID.c_str()).first;
            // LOG(debug) << "owner (" << ownerID << ") use count: " << owner->fPtr.use_count();


            // if (owner)
            // {
            //     // void* ptr = owner->fPtr->GetData();

            //     // LOG(debug) << "chunk handle: " << owner->fPtr->GetHandle();
            //     // LOG(debug) << "chunk size: " << owner->fPtr->GetSize();

            //     fBytesInNew += owner->fPtr->GetSize();
            //     ++fMsgInNew;

            //     // char* cptr = static_cast<char*>(ptr);
            //     // LOG(debug) << "check: " << cptr[3];

            //     SegmentManager::Instance().Segment()->deallocate(ptr);

            //     // SegmentManager::Instance().Segment()->destroy_ptr(owner);
            // }
            // else
            // {
            //     LOG(warn) << "Shared pointer is zero.";
            // }


            ++numReceivedMsgs;
        }
    }

    LOG(info) << "Received " << numReceivedMsgs << " messages, leaving RUNNING state.";

    rateLogger.join();
}

void FairMQShmPrototypeSink::Log(const int intervalInMs)
{
    timestamp_t t0 = get_timestamp();
    timestamp_t t1;
    timestamp_t msSinceLastLog;

    double mbPerSecIn = 0;
    double msgPerSecIn = 0;

    while (CheckCurrentState(RUNNING))
    {
        t1 = get_timestamp();

        msSinceLastLog = (t1 - t0) / 1000.0L;

        mbPerSecIn = (static_cast<double>(fBytesInNew - fBytesIn) / (1024. * 1024.)) / static_cast<double>(msSinceLastLog) * 1000.;
        fBytesIn = fBytesInNew;

        msgPerSecIn = static_cast<double>(fMsgInNew - fMsgIn) / static_cast<double>(msSinceLastLog) * 1000.;
        fMsgIn = fMsgInNew;

        LOG(debug) << fixed
                   << setprecision(0) << "in: " << msgPerSecIn << " msg ("
                   << setprecision(2) << mbPerSecIn << " MB)\t("
                   << SegmentManager::Instance().Segment()->get_free_memory() / (1024. * 1024.) << " MB free)";

        t0 = t1;
        this_thread::sleep_for(chrono::milliseconds(intervalInMs));
    }
}
