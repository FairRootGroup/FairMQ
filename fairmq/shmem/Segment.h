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

#include <boost/variant.hpp>

#include <cstdint>
#include <string>

namespace fair::mq::shmem
{

struct SimpleSeqFit {};
struct RBTreeBestFit {};
static const SimpleSeqFit simpleSeqFit = SimpleSeqFit();
static const RBTreeBestFit rbTreeBestFit = RBTreeBestFit();

struct Segment
{
    Segment(const std::string& shmId, uint16_t id, size_t size, SimpleSeqFit)
        : fSegment(SimpleSeqFitSegment(boost::interprocess::open_or_create,
                                       std::string("fmq_" + shmId + "_m_" + std::to_string(id)).c_str(),
                                       size))
    {
        Register(shmId, id, AllocationAlgorithm::simple_seq_fit);
    }

    Segment(const std::string& shmId, uint16_t id, size_t size, RBTreeBestFit)
        : fSegment(RBTreeBestFitSegment(boost::interprocess::open_or_create,
                                        std::string("fmq_" + shmId + "_m_" + std::to_string(id)).c_str(),
                                        size))
    {
        Register(shmId, id, AllocationAlgorithm::rbtree_best_fit);
    }

    size_t GetSize() const { return boost::apply_visitor(SegmentSize(), fSegment); }
    void* GetData() { return boost::apply_visitor(SegmentAddress(), fSegment); }

    size_t GetFreeMemory() const { return boost::apply_visitor(SegmentFreeMemory(), fSegment); }

    void Zero() { boost::apply_visitor(SegmentMemoryZeroer(), fSegment); }
    void Lock()
    {
        if (mlock(GetData(), GetSize()) == -1) {
            throw TransportError(tools::ToString("Could not lock the managed segment memory: ", strerror(errno)));
        }
    }

    static void Remove(const std::string& shmId, uint16_t id)
    {
        Monitor::RemoveObject("fmq_" + shmId + "_m_" + std::to_string(id));
    }

  private:
    boost::variant<RBTreeBestFitSegment, SimpleSeqFitSegment> fSegment;

    static void Register(const std::string& shmId, uint16_t id, AllocationAlgorithm allocAlgo)
    {
        using namespace boost::interprocess;
        managed_shared_memory mngSegment(open_or_create, std::string("fmq_" + shmId + "_mng").c_str(), 6553600);
        VoidAlloc alloc(mngSegment.get_segment_manager());

        Uint16SegmentInfoHashMap* shmSegments = mngSegment.find_or_construct<Uint16SegmentInfoHashMap>(unique_instance)(alloc);

        EventCounter* eventCounter = mngSegment.find<EventCounter>(unique_instance).first;
        if (!eventCounter) {
            eventCounter = mngSegment.construct<EventCounter>(unique_instance)(0);
        }

        bool newSegmentRegistered = shmSegments->emplace(id, allocAlgo).second;
        if (newSegmentRegistered) {
            (eventCounter->fCount)++;
        }
    }
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_SEGMENT_H_ */
