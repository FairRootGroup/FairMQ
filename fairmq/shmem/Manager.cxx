/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Manager.h"

#include <fairmq/tools/CppSTL.h>
#include <fairmq/tools/Strings.h>

#include <boost/process.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using bie = ::boost::interprocess::interprocess_exception;
namespace bipc = ::boost::interprocess;
namespace bfs = ::boost::filesystem;
namespace bpt = ::boost::posix_time;

namespace fair
{
namespace mq
{
namespace shmem
{

Manager::Manager(const string& id, size_t size)
    : fShmId(id)
    , fSegmentName("fmq_" + fShmId + "_main")
    , fManagementSegmentName("fmq_" + fShmId + "_mng")
    , fSegment(bipc::open_or_create, fSegmentName.c_str(), size)
    , fManagementSegment(bipc::open_or_create, fManagementSegmentName.c_str(), 655360)
    , fShmVoidAlloc(fManagementSegment.get_segment_manager())
    , fShmMtx(bipc::open_or_create, string("fmq_" + fShmId + "_mtx").c_str())
    , fRegionEventsCV(bipc::open_or_create, string("fmq_" + fShmId + "_cv").c_str())
    , fRegionEventsSubscriptionActive(false)
    , fDeviceCounter(nullptr)
    , fRegionInfos(nullptr)
    , fInterrupted(false)
{
    LOG(debug) << "created/opened shared memory segment '" << "fmq_" << fShmId << "_main" << "' of " << size << " bytes. Available are " << fSegment.get_free_memory() << " bytes.";

    fRegionInfos = fManagementSegment.find_or_construct<Uint64RegionInfoMap>(bipc::unique_instance)(fShmVoidAlloc);
    // store info about the managed segment as region with id 0
    fRegionInfos->emplace(0, RegionInfo("", 0, 0, fShmVoidAlloc));

    bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);

    fDeviceCounter = fManagementSegment.find<DeviceCounter>(bipc::unique_instance).first;

    if (fDeviceCounter) {
        LOG(debug) << "device counter found, with value of " << fDeviceCounter->fCount << ". incrementing.";
        (fDeviceCounter->fCount)++;
        LOG(debug) << "incremented device counter, now: " << fDeviceCounter->fCount;
    } else {
        LOG(debug) << "no device counter found, creating one and initializing with 1";
        fDeviceCounter = fManagementSegment.construct<DeviceCounter>(bipc::unique_instance)(1);
        LOG(debug) << "initialized device counter with: " << fDeviceCounter->fCount;
    }
}

void Manager::StartMonitor(const string& id)
{
    try {
        bipc::named_mutex monitorStatus(bipc::open_only, string("fmq_" + id + "_ms").c_str());
        LOG(debug) << "Found fairmq-shmmonitor for shared memory id " << id;
    } catch (bie&) {
        LOG(debug) << "no fairmq-shmmonitor found for shared memory id " << id << ", starting...";
        auto env = boost::this_process::environment();

        vector<bfs::path> ownPath = boost::this_process::path();

        if (const char* fmqp = getenv("FAIRMQ_PATH")) {
            ownPath.insert(ownPath.begin(), bfs::path(fmqp));
        }

        bfs::path p = boost::process::search_path("fairmq-shmmonitor", ownPath);

        if (!p.empty()) {
            boost::process::spawn(p, "-x", "--shmid", id, "-d", "-t", "2000", env);
            int numTries = 0;
            do {
                try {
                    bipc::named_mutex monitorStatus(bipc::open_only, string("fmq_" + id + "_ms").c_str());
                    LOG(debug) << "Started fairmq-shmmonitor for shared memory id " << id;
                    break;
                } catch (bie&) {
                    this_thread::sleep_for(chrono::milliseconds(10));
                    if (++numTries > 1000) {
                        LOG(error) << "Did not get response from fairmq-shmmonitor after " << 10 * 1000 << " milliseconds. Exiting.";
                        throw runtime_error(tools::ToString("Did not get response from fairmq-shmmonitor after ", 10 * 1000, " milliseconds. Exiting."));
                    }
                }
            } while (true);
        } else {
            LOG(warn) << "could not find fairmq-shmmonitor in the path";
        }
    }
}

pair<bipc::mapped_region*, uint64_t> Manager::CreateRegion(const size_t size, const int64_t userFlags, RegionCallback callback, const string& path /* = "" */, int flags /* = 0 */)
{
    try {

        pair<bipc::mapped_region*, uint64_t> result;

        {
            uint64_t id = 0;
            bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);

            RegionCounter* rc = fManagementSegment.find<RegionCounter>(bipc::unique_instance).first;

            if (rc) {
                LOG(debug) << "region counter found, with value of " << rc->fCount << ". incrementing.";
                (rc->fCount)++;
                LOG(debug) << "incremented region counter, now: " << rc->fCount;
            } else {
                LOG(debug) << "no region counter found, creating one and initializing with 1";
                rc = fManagementSegment.construct<RegionCounter>(bipc::unique_instance)(1);
                LOG(debug) << "initialized region counter with: " << rc->fCount;
            }

            id = rc->fCount;

            auto it = fRegions.find(id);
            if (it != fRegions.end()) {
                LOG(error) << "Trying to create a region that already exists";
                return {nullptr, id};
            }

            // create region info
            fRegionInfos->emplace(id, RegionInfo(path.c_str(), flags, userFlags, fShmVoidAlloc));

            auto r = fRegions.emplace(id, tools::make_unique<Region>(*this, id, size, false, callback, path, flags));
            // LOG(debug) << "Created region with id '" << id << "', path: '" << path << "', flags: '" << flags << "'";

            r.first->second->StartReceivingAcks();
            result.first = &(r.first->second->fRegion);
            result.second = id;
        }
        fRegionEventsCV.notify_all();

        return result;

    } catch (bipc::interprocess_exception& e) {
        LOG(error) << "cannot create region. Already created/not cleaned up?";
        LOG(error) << e.what();
        throw;
    }
}

void Manager::RemoveRegion(const uint64_t id)
{
    {
        bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);
        fRegions.erase(id);
        fRegionInfos->at(id).fDestroyed = true;
    }
    fRegionEventsCV.notify_all();
}

Region* Manager::GetRegion(const uint64_t id)
{
    bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);
    return GetRegionUnsafe(id);
}

Region* Manager::GetRegionUnsafe(const uint64_t id)
{
    // remote region could actually be a local one if a message originates from this device (has been sent out and returned)
    auto it = fRegions.find(id);
    if (it != fRegions.end()) {
        return it->second.get();
    } else {
        try {
            // get region info
            RegionInfo regionInfo = fRegionInfos->at(id);
            string path = regionInfo.fPath.c_str();
            int flags = regionInfo.fFlags;
            // LOG(debug) << "Located remote region with id '" << id << "', path: '" << path << "', flags: '" << flags << "'";

            auto r = fRegions.emplace(id, tools::make_unique<Region>(*this, id, 0, true, nullptr, path, flags));
            r.first->second->StartSendingAcks();
            return r.first->second.get();
        } catch (bie& e) {
            LOG(warn) << "Could not get remote region for id: " << id;
            return nullptr;
        }
    }
}

vector<fair::mq::RegionInfo> Manager::GetRegionInfo()
{
    bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);
    return GetRegionInfoUnsafe();
}

vector<fair::mq::RegionInfo> Manager::GetRegionInfoUnsafe()
{
    vector<fair::mq::RegionInfo> result;

    for (const auto& e : *fRegionInfos) {
        fair::mq::RegionInfo info;
        info.id = e.first;
        info.flags = e.second.fUserFlags;
        info.event = e.second.fDestroyed ? RegionEvent::destroyed : RegionEvent::created;
        if (info.id != 0) {
            if (!e.second.fDestroyed) {
                auto region = GetRegionUnsafe(info.id);
                info.ptr = region->fRegion.get_address();
                info.size = region->fRegion.get_size();
            } else {
                info.ptr = nullptr;
                info.size = 0;
            }
            result.push_back(info);
        } else {
            if (!e.second.fDestroyed) {
                info.ptr = fSegment.get_address();
                info.size = fSegment.get_size();
            } else {
                info.ptr = nullptr;
                info.size = 0;
            }
            result.push_back(info);
        }
    }

    return result;
}

void Manager::SubscribeToRegionEvents(RegionEventCallback callback)
{
    bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);
    if (fRegionEventThread.joinable()) {
        fRegionEventsSubscriptionActive.store(false);
        fRegionEventThread.join();
    }
    fRegionEventCallback = callback;
    fRegionEventsSubscriptionActive.store(true);
    fRegionEventThread = thread(&Manager::RegionEventsSubscription, this);
}

void Manager::UnsubscribeFromRegionEvents()
{
    if (fRegionEventThread.joinable()) {
        fRegionEventsSubscriptionActive.store(false);
        fRegionEventsCV.notify_all();
        fRegionEventThread.join();
    }
    bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);
    fRegionEventCallback = nullptr;
}

void Manager::RegionEventsSubscription()
{
    while (fRegionEventsSubscriptionActive.load()) {
        bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);
        auto infos = GetRegionInfoUnsafe();
        for (const auto& i : infos) {
            auto el = fObservedRegionEvents.find(i.id);
            if (el == fObservedRegionEvents.end()) {
                fRegionEventCallback(i);
                fObservedRegionEvents.emplace(i.id, i.event);
            } else {
                if (el->second == RegionEvent::created && i.event == RegionEvent::destroyed) {
                    fRegionEventCallback(i);
                    el->second = i.event;
                } else {
                    // LOG(debug) << "ignoring event for id" << i.id << ":";
                    // LOG(debug) << "incoming event: " << i.event;
                    // LOG(debug) << "stored event: " << el->second;
                }
            }
        }
        fRegionEventsCV.wait(lock);
    }
}

void Manager::RemoveSegments()
{
    if (bipc::shared_memory_object::remove(fSegmentName.c_str())) {
        LOG(debug) << "successfully removed '" << fSegmentName << "' segment after the device has stopped.";
    } else {
        LOG(debug) << "did not remove " << fSegmentName << " segment after the device stopped. Already removed?";
    }

    if (bipc::shared_memory_object::remove(fManagementSegmentName.c_str())) {
        LOG(debug) << "successfully removed '" << fManagementSegmentName << "' segment after the device has stopped.";
    } else {
        LOG(debug) << "did not remove '" << fManagementSegmentName << "' segment after the device stopped. Already removed?";
    }
}

Manager::~Manager()
{
    bool lastRemoved = false;

    if (fRegionEventThread.joinable()) {
        fRegionEventsSubscriptionActive.store(false);
        fRegionEventsCV.notify_all();
        fRegionEventThread.join();
    }

    try {
        bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);

        (fDeviceCounter->fCount)--;

        if (fDeviceCounter->fCount == 0) {
            LOG(debug) << "last segment user, removing segment.";

            RemoveSegments();
            lastRemoved = true;
        } else {
            LOG(debug) << "other segment users present (" << fDeviceCounter->fCount << "), not removing it.";
        }
    } catch(bie& e) {
        LOG(error) << "error while acquiring lock in Manager destructor: " << e.what();
    }

    if (lastRemoved) {
        bipc::named_mutex::remove(string("fmq_" + fShmId + "_mtx").c_str());
        bipc::named_condition::remove(string("fmq_" + fShmId + "_cv").c_str());
    }
}

} // namespace shmem
} // namespace mq
} // namespace fair
