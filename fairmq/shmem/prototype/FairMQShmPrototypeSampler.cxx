/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQShmPrototypeSampler.cpp
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

#include "FairMQShmPrototypeSampler.h"
#include "FairMQProgOptions.h"
#include "FairMQLogger.h"

#include "ShmChunk.h"

using namespace std;
using namespace boost::interprocess;

FairMQShmPrototypeSampler::FairMQShmPrototypeSampler()
    : fMsgSize(10000)
    , fMsgCounter(0)
    , fMsgRate(1)
    , fBytesOut(0)
    , fMsgOut(0)
    , fBytesOutNew(0)
    , fMsgOutNew(0)
{
    if (shared_memory_object::remove("FairMQSharedMemoryPrototype"))
    {
        LOG(INFO) << "Successfully removed shared memory upon device start.";
    }
    else
    {
        LOG(INFO) << "Did not remove shared memory upon device start.";
    }
}

FairMQShmPrototypeSampler::~FairMQShmPrototypeSampler()
{
    if (shared_memory_object::remove("FairMQSharedMemoryPrototype"))
    {
        LOG(INFO) << "Successfully removed shared memory after the device has stopped.";
    }
    else
    {
        LOG(INFO) << "Did not remove shared memory after the device stopped. Still in use?";
    }
}

void FairMQShmPrototypeSampler::Init()
{
    fMsgSize = fConfig->GetValue<int>("msg-size");
    fMsgRate = fConfig->GetValue<int>("msg-rate");

    SegmentManager::Instance().InitializeSegment("open_or_create", "FairMQSharedMemoryPrototype", 2000000000);
    LOG(INFO) << "Created/Opened shared memory segment of 2,000,000,000 bytes. Available are "
              << SegmentManager::Instance().Segment()->get_free_memory() << " bytes.";
}

void FairMQShmPrototypeSampler::Run()
{
    // count sent messages (also used in creating ShmChunk container ID)
    static uint64_t numSentMsgs = 0;

    LOG(INFO) << "Starting the benchmark with message size of " << fMsgSize;

    // start rate logger and acknowledgement listener in separate threads
    thread rateLogger(&FairMQShmPrototypeSampler::Log, this, 1000);
    // thread resetMsgCounter(&FairMQShmPrototypeSampler::ResetMsgCounter, this);

    // int charnum = 97;

    while (CheckCurrentState(RUNNING))
    {
        void* ptr = nullptr;
        bipc::managed_shared_memory::handle_t handle;

        while (!ptr)
        {
            try
            {
                ptr = SegmentManager::Instance().Segment()->allocate(fMsgSize);
            }
            catch (bipc::bad_alloc& ba)
            {
                this_thread::sleep_for(chrono::milliseconds(50));
                if (CheckCurrentState(RUNNING))
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        // // ShmChunk container ID
        // string chunkID = "c" + to_string(numSentMsgs);
        // // shared pointer ID
        // string ownerID = "o" + to_string(numSentMsgs);

        // ShPtrOwner* owner = nullptr;

        // try
        // {
        //     owner = SegmentManager::Instance().Segment()->construct<ShPtrOwner>(ownerID.c_str())(
        //         make_managed_shared_ptr(SegmentManager::Instance().Segment()->construct<ShmChunk>(chunkID.c_str())(fMsgSize),
        //                                 *(SegmentManager::Instance().Segment())));
        // }
        // catch (bipc::bad_alloc& ba)
        // {
        //     LOG(WARN) << "Shared memory full...";
        //     this_thread::sleep_for(chrono::milliseconds(100));
        //     continue;
        // }

        // void* ptr = owner->fPtr->GetData();

        // write something to memory, otherwise only (incomplete) allocation will be measured
        // memset(ptr, 0, fMsgSize);

        // static_cast<char*>(ptr)[3] = charnum++;
        // if (charnum == 123)
        // {
        //     charnum = 97;
        // }

        // LOG(DEBUG) << "chunk handle: " << owner->fPtr->GetHandle();
        // LOG(DEBUG) << "chunk size: " << owner->fPtr->GetSize();
        // LOG(DEBUG) << "owner (" << ownerID << ") use count: " << owner->fPtr.use_count();

        // char* cptr = static_cast<char*>(ptr);
        // LOG(DEBUG) << "check: " << cptr[3];

        // FairMQMessagePtr msg(NewSimpleMessage(ownerID));

        if (ptr)
        {
            handle = SegmentManager::Instance().Segment()->get_handle_from_address(ptr);
            FairMQMessagePtr msg(NewMessage(sizeof(ExMetaHeader)));
            ExMetaHeader* metaPtr = new(msg->GetData()) ExMetaHeader();
            metaPtr->fSize = fMsgSize;
            metaPtr->fHandle = handle;

            // LOG(INFO) << metaPtr->fSize;
            // LOG(INFO) << metaPtr->fHandle;
            // LOG(WARN) << ptr;

            if (Send(msg, "meta", 0) > 0)
            {
                fBytesOutNew += fMsgSize;
                ++fMsgOutNew;
                ++numSentMsgs;
            }
            else
            {
                SegmentManager::Instance().Segment()->deallocate(ptr);
                // SegmentManager::Instance().Segment()->destroy_ptr(owner);
            }
        }

        // --fMsgCounter;
        // while (fMsgCounter == 0)
        // {
        //     this_thread::sleep_for(chrono::milliseconds(1));
        // }
    }

    LOG(INFO) << "Sent " << numSentMsgs << " messages, leaving RUNNING state.";

    rateLogger.join();
    // resetMsgCounter.join();
}

void FairMQShmPrototypeSampler::Log(const int intervalInMs)
{
    timestamp_t t0 = get_timestamp();
    timestamp_t t1;
    timestamp_t msSinceLastLog;

    double mbPerSecOut = 0;
    double msgPerSecOut = 0;

    while (CheckCurrentState(RUNNING))
    {
        t1 = get_timestamp();

        msSinceLastLog = (t1 - t0) / 1000.0L;

        mbPerSecOut = (static_cast<double>(fBytesOutNew - fBytesOut) / (1024. * 1024.)) / static_cast<double>(msSinceLastLog) * 1000.;
        fBytesOut = fBytesOutNew;

        msgPerSecOut = static_cast<double>(fMsgOutNew - fMsgOut) / static_cast<double>(msSinceLastLog) * 1000.;
        fMsgOut = fMsgOutNew;

        LOG(DEBUG) << fixed
                   << setprecision(0) << "out: " << msgPerSecOut << " msg ("
                   << setprecision(2) << mbPerSecOut << " MB)\t("
                   << SegmentManager::Instance().Segment()->get_free_memory() / (1024. * 1024.) << " MB free)";

        t0 = t1;
        this_thread::sleep_for(chrono::milliseconds(intervalInMs));
    }
}

void FairMQShmPrototypeSampler::ResetMsgCounter()
{
    while (CheckCurrentState(RUNNING))
    {
        fMsgCounter = fMsgRate / 100;
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}
