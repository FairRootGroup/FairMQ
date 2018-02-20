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

#include <zmq.h>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

Message::Message()
{
}

Message::Message(const size_t size)
    : fSize{size}
{
    throw MessageError{"Not yet implemented."};
}

Message::Message(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    throw MessageError{"Not yet implemented."};
}

Message::Message(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint)
{
    throw MessageError{"Not yet implemented."};
}

auto Message::Rebuild() -> void
{
    throw MessageError{"Not implemented."};
}

auto Message::Rebuild(const size_t size) -> void
{
    throw MessageError{"Not implemented."};
}

auto Message::Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) -> void
{
    throw MessageError{"Not implemented."};
}

auto Message::GetData() const -> void*
{
    throw MessageError{"Not implemented."};
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

auto Message::Copy(const fair::mq::Message& msg) -> void
{
    throw MessageError{"Not yet implemented."};
}

auto Message::Copy(const fair::mq::MessagePtr& msg) -> void
{
    throw MessageError{"Not yet implemented."};
}

Message::~Message() noexcept(false)
{
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
