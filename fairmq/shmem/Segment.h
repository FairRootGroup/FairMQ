/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_SEGMENT_H_
#define FAIR_MQ_SHMEM_SEGMENT_H_

#include <fairmq/shmem/Common.h>
#include <fairmq/shmem/Monitor.h>

#include <cstdint>
#include <string>
#include <variant>

namespace fair::mq::shmem
{

struct SimpleSeqFit {};
struct RBTreeBestFit {};
static const SimpleSeqFit simpleSeqFit = SimpleSeqFit();
static const RBTreeBestFit rbTreeBestFit = RBTreeBestFit();

struct Segment
{
    friend class Monitor;

    Segment(const std::string& shmId, uint16_t id, size_t size, SimpleSeqFit)
        : fSegment(SimpleSeqFitSegment(boost::interprocess::open_or_create, MakeShmName(shmId, "m", id).c_str(), size))
    {
        Register(shmId, id, AllocationAlgorithm::simple_seq_fit);
    }

    Segment(const std::string& shmId, uint16_t id, size_t size, RBTreeBestFit)
        : fSegment(RBTreeBestFitSegment(boost::interprocess::open_or_create, MakeShmName(shmId, "m", id).c_str(), size))
    {
        Register(shmId, id, AllocationAlgorithm::rbtree_best_fit);
    }

    size_t GetSize() const { return std::visit([](auto& s){ return s.get_size(); }, fSegment); }
    void* GetData() { return std::visit([](auto& s){ return s.get_address(); }, fSegment); }

    size_t GetFreeMemory() const { return std::visit([](auto& s){ return s.get_free_memory(); }, fSegment); }

    void Zero() { std::visit([](auto& s){ return s.zero_free_memory(); }, fSegment); }
    void Lock()
    {
        if (mlock(GetData(), GetSize()) == -1) {
            throw TransportError(tools::ToString("Could not lock the managed segment memory: ", strerror(errno)));
        }
    }

    static void Remove(const std::string& shmId, uint16_t id)
    {
        Monitor::RemoveObject(MakeShmName(shmId, "m", id));
    }

  private:
    std::variant<RBTreeBestFitSegment, SimpleSeqFitSegment> fSegment;

    static void Register(const std::string& shmId, uint16_t id, AllocationAlgorithm allocAlgo)
    {
        using namespace boost::interprocess;
        managed_shared_memory mngSegment(open_or_create, MakeShmName(shmId, "mng").c_str(), kManagementSegmentSize);
        VoidAlloc alloc(mngSegment.get_segment_manager());

        Uint16SegmentInfoHashMap* shmSegments = mngSegment.find_or_construct<Uint16SegmentInfoHashMap>(unique_instance)(alloc);

        EventCounter* eventCounter = mngSegment.find_or_construct<EventCounter>(unique_instance)(0);

        bool newSegmentRegistered = shmSegments->emplace(id, allocAlgo).second;
        if (newSegmentRegistered) {
            (eventCounter->fCount)++;
        }
    }
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_SEGMENT_H_ */
