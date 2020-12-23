/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/Message.h>
#include <fairmq/Tools.h>
#include <FairMQLogger.h>

#include <asiofi.hpp>
#include <cassert>
#include <cstdlib>
#include <zmq.h>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

Message::Message(boost::container::pmr::memory_resource* pmr)
    : fInitialSize(0)
    , fSize(0)
    , fData(nullptr)
    , fFreeFunction(nullptr)
    , fHint(nullptr)
    , fPmr(pmr)
{
}

Message::Message(boost::container::pmr::memory_resource* pmr, Alignment /* alignment */)
    : fInitialSize(0)
    , fSize(0)
    , fData(nullptr)
    , fFreeFunction(nullptr)
    , fHint(nullptr)
    , fPmr(pmr)
{
}

Message::Message(boost::container::pmr::memory_resource* pmr, const size_t size)
    : fInitialSize(size)
    , fSize(size)
    , fData(nullptr)
    , fFreeFunction(nullptr)
    , fHint(nullptr)
    , fPmr(pmr)
{
    if (size) {
        fData = fPmr->allocate(size);
        assert(fData);
    }
}

Message::Message(boost::container::pmr::memory_resource* pmr, const size_t size, Alignment /* alignment */)
    : fInitialSize(size)
    , fSize(size)
    , fData(nullptr)
    , fFreeFunction(nullptr)
    , fHint(nullptr)
    , fPmr(pmr)
{
    if (size) {
        fData = fPmr->allocate(size);
        assert(fData);
    }
}

Message::Message(boost::container::pmr::memory_resource* pmr,
                 void* data,
                 const size_t size,
                 fairmq_free_fn* ffn,
                 void* hint)
    : fInitialSize(size)
    , fSize(size)
    , fData(data)
    , fFreeFunction(ffn)
    , fHint(hint)
    , fPmr(pmr)
{}

Message::Message(boost::container::pmr::memory_resource* /*pmr*/,
                 FairMQUnmanagedRegionPtr& /*region*/,
                 void* /*data*/,
                 const size_t /*size*/,
                 void* /*hint*/)
{
    throw MessageError{"Not yet implemented."};
}

auto Message::Rebuild() -> void
{
    if (fFreeFunction) {
        fFreeFunction(fData, fHint);
    } else {
        if (fData) {
            fPmr->deallocate(fData, fSize);
        }
    }
    fData = nullptr;
    fInitialSize = 0;
    fSize = 0;
    fFreeFunction = nullptr;
    fHint = nullptr;
}

auto Message::Rebuild(Alignment /* alignment */) -> void
{
    // TODO: implement alignment
    Rebuild();
}

auto Message::Rebuild(const size_t size) -> void
{
    if (fFreeFunction) {
      fFreeFunction(fData, fHint);
    } else {
        if (fData) {
            fPmr->deallocate(fData, fSize);
        }
    }
    if (size) {
        fData = fPmr->allocate(size);
        assert(fData);
    } else {
        fData = nullptr;
    }
    fInitialSize = size;
    fSize = size;
    fFreeFunction = nullptr;
    fHint = nullptr;
}

auto Message::Rebuild(const size_t size, Alignment /* alignment */) -> void
{
    // TODO: implement alignment
    Rebuild(size);
}

auto Message::Rebuild(void* /*data*/, const size_t size, fairmq_free_fn* ffn, void* hint) -> void
{
    if (fFreeFunction) {
      fFreeFunction(fData, fHint);
    } else {
        if (fData) {
            fPmr->deallocate(fData, fSize);
        }
    }
    if (size) {
        fData = fPmr->allocate(size);
        assert(fData);
    } else {
        fData = nullptr;
    }
    assert(fData);
    fInitialSize = size;
    fSize = size;
    fFreeFunction = ffn;
    fHint = hint;
}

auto Message::GetData() const -> void*
{
    return fData;
}

auto Message::GetSize() const -> size_t
{
    return fSize;
}

auto Message::SetUsedSize(const size_t size) -> bool
{
    if (size == fSize) {
        return true;
    } else if (size <= fSize) {
        throw MessageError{"Message size shrinking not yet implemented."};
    } else {
        throw MessageError{"Cannot grow message size."};
    }
}

auto Message::Copy(const fair::mq::Message& /*msg*/) -> void
{
    throw MessageError{"Not yet implemented."};
}

Message::~Message()
{
    if (fFreeFunction) {
        fFreeFunction(fData, fHint);
    } else {
        if (fData) {
            fPmr->deallocate(fData, fSize);
        }
    }
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
