/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Manager.h"
#include "Common.h"

#include <fairmq/tools/CppSTL.h>
#include <fairmq/tools/Strings.h>

#include <boost/process.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using bie = ::boost::interprocess::interprocess_exception;
namespace bipc = ::boost::interprocess;
namespace bfs = ::boost::filesystem;

namespace fair
{
namespace mq
{
namespace shmem
{

std::unordered_map<uint64_t, std::unique_ptr<Region>> Manager::fRegions;

Manager::Manager(const std::string& id, size_t size)
    : fShmId(id)
    , fSegmentName("fmq_" + fShmId + "_main")
    , fManagementSegmentName("fmq_" + fShmId + "_mng")
    , fSegment(bipc::open_or_create, fSegmentName.c_str(), size)
    , fManagementSegment(bipc::open_or_create, fManagementSegmentName.c_str(), 65536)
    , fShmMtx(bipc::open_or_create, string("fmq_" + fShmId + "_mtx").c_str())
    , fDeviceCounter(nullptr)
{
    LOG(debug) << "created/opened shared memory segment '" << "fmq_" << fShmId << "_main" << "' of " << size << " bytes. Available are " << fSegment.get_free_memory() << " bytes.";

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

void Manager::StartMonitor(const std::string& id)
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

void Manager::Interrupt()
{
}

void Manager::Resume()
{
    // close remote regions before processing new transfers
    for (auto it = fRegions.begin(); it != fRegions.end(); /**/) {
        if (it->second->fRemote) {
           it = fRegions.erase(it);
        } else {
            ++it;
        }
    }
}

bipc::mapped_region* Manager::CreateRegion(const size_t size, const uint64_t id, RegionCallback callback, const std::string& path /* = "" */, int flags /* = 0 */)
{
    auto it = fRegions.find(id);
    if (it != fRegions.end()) {
        LOG(error) << "Trying to create a region that already exists";
        return nullptr;
    } else {
        // create region info
        {
            bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);
            VoidAlloc voidAlloc(fManagementSegment.get_segment_manager());
            Uint64RegionInfoMap* infoMap = fManagementSegment.find_or_construct<Uint64RegionInfoMap>(bipc::unique_instance)(voidAlloc);
            infoMap->emplace(id, RegionInfo(path.c_str(), flags, voidAlloc));
        }
        // LOG(debug) << "Created region with id '" << id << "', path: '" << path << "', flags: '" << flags << "'";

        auto r = fRegions.emplace(id, tools::make_unique<Region>(*this, id, size, false, callback, path, flags));

        r.first->second->StartReceivingAcks();

        return &(r.first->second->fRegion);
    }
}

Region* Manager::GetRemoteRegion(const uint64_t id)
{
    // remote region could actually be a local one if a message originates from this device (has been sent out and returned)
    auto it = fRegions.find(id);
    if (it != fRegions.end()) {
        return it->second.get();
    } else {
        try {
            string path;
            int flags;

            // get region info
            {
                bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);
                Uint64RegionInfoMap* infoMap = fManagementSegment.find<Uint64RegionInfoMap>(bipc::unique_instance).first;
                if (infoMap == nullptr) {
                    LOG(error) << "Unable to locate the region info";
                    throw SharedMemoryError("Unable to locate remote region info");
                }
                RegionInfo regionInfo = infoMap->at(id);
                path = regionInfo.fPath.c_str();
                flags = regionInfo.fFlags;
            }
            // LOG(debug) << "Located remote region with id '" << id << "', path: '" << path << "', flags: '" << flags << "'";

            auto r = fRegions.emplace(id, tools::make_unique<Region>(*this, id, 0, true, nullptr, path, flags));
            return r.first->second.get();
        } catch (bie& e) {
            LOG(warn) << "Could not get remote region for id: " << id;
            return nullptr;
        }

    }
}

void Manager::RemoveRegion(const uint64_t id)
{
    fRegions.erase(id);
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

    {
        bipc::scoped_lock<bipc::named_mutex> lock(fShmMtx);

        (fDeviceCounter->fCount)--;

        if (fDeviceCounter->fCount == 0) {
            LOG(debug) << "last segment user, removing segment.";

            RemoveSegments();
            lastRemoved = true;
        } else {
            LOG(debug) << "other segment users present (" << fDeviceCounter->fCount << "), not removing it.";
        }
    }

    if (lastRemoved) {
        bipc::named_mutex::remove(string("fmq_" + fShmId + "_mtx").c_str());
    }
}

} // namespace shmem
} // namespace mq
} // namespace fair
