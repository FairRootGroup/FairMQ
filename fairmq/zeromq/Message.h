/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_ZMQ_MESSAGE_H
#define FAIR_MQ_ZMQ_MESSAGE_H

#include <fairmq/zeromq/UnmanagedRegion.h>
#include <fairmq/Message.h>
#include <fairmq/UnmanagedRegion.h>

#include <fairlogger/Logger.h>

#include <zmq.h>

#include <cstddef>
#include <cstdlib> // malloc
#include <cstring>
#include <memory> // make_unique
#include <new> // bad_alloc
#include <string>

namespace fair::mq::zmq
{

class Socket;

class Message final : public fair::mq::Message
{
    friend class Socket;

  public:
    Message(const Message&) = delete;
    Message(Message&&) = delete;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) = delete;

    Message(fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fMsg(std::make_unique<zmq_msg_t>())
    {
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }

    Message(Alignment alignment, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fAlignment(alignment.alignment)
        , fMsg(std::make_unique<zmq_msg_t>())
    {
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }

    Message(const size_t size, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fMsg(std::make_unique<zmq_msg_t>())
    {
        if (zmq_msg_init_size(fMsg.get(), size) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }
    }

    static std::pair<void*, void*> AllocateAligned(size_t size, size_t alignment)
    {
        char* fullBufferPtr = static_cast<char*>(malloc(size + alignment));
        if (!fullBufferPtr) {
            LOG(error) << "failed to allocate buffer with provided size (" << size << ") and alignment (" << alignment << ").";
            throw std::bad_alloc();
        }

        size_t offset = alignment - (reinterpret_cast<uintptr_t>(fullBufferPtr) % alignment);
        char* alignedPartPtr = fullBufferPtr + offset;

        return {static_cast<void*>(fullBufferPtr), static_cast<void*>(alignedPartPtr)};
    }

    Message(const size_t size, Alignment alignment, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fAlignment(alignment.alignment)
        , fMsg(std::make_unique<zmq_msg_t>())
    {
        if (fAlignment != 0) {
            auto ptrs = AllocateAligned(size, fAlignment);
            if (zmq_msg_init_data(fMsg.get(), ptrs.second, size, [](void* /* data */, void* hint) { free(hint); }, ptrs.first) != 0) {
                LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
            }
        } else {
            if (zmq_msg_init_size(fMsg.get(), size) != 0) {
                LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
            }
        }
    }

    Message(void* data, const size_t size, fair::mq::FreeFn* ffn, void* hint = nullptr, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fMsg(std::make_unique<zmq_msg_t>())
    {
        if (zmq_msg_init_data(fMsg.get(), data, size, ffn, hint) != 0) {
            LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
        }
    }

    Message(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fMsg(std::make_unique<zmq_msg_t>())
    {
        if (region->GetType() != GetType()) {
            LOG(error) << "region type (" << region->GetType() << ") does not match message type (" << GetType() << ")";
            throw TransportError(tools::ToString("region type (", region->GetType(), ") does not match message type (", GetType(), ")"));
        }

        // FIXME: make this zero-copy:
        // simply taking over the provided buffer can casue premature delete, since region could be
        // destroyed before the message is sent out. Needs lifetime extension for the ZMQ region.
        if (zmq_msg_init_size(fMsg.get(), size) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }

        std::memcpy(zmq_msg_data(fMsg.get()), data, size);
        // call region callback
        auto ptr = static_cast<UnmanagedRegion*>(region.get());
        if (ptr->fBulkCallback) {
            ptr->fBulkCallback({{data, size, hint}});
        } else if (ptr->fCallback) {
            ptr->fCallback(data, size, hint);
        }

        // if (zmq_msg_init_data(fMsg.get(), data, size, [](void*, void*){}, nullptr) != 0)
        // {
        //     LOG(error) << "failed initializing message with data, reason: " <<
        //     zmq_strerror(errno);
        // }
    }

    void Rebuild() override
    {
        CloseMessage();
        fMsg = std::make_unique<zmq_msg_t>();
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }

    void Rebuild(Alignment alignment) override
    {
        CloseMessage();
        fAlignment = alignment.alignment;
        fMsg = std::make_unique<zmq_msg_t>();
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }

    void Rebuild(size_t size) override
    {
        CloseMessage();
        fMsg = std::make_unique<zmq_msg_t>();
        if (zmq_msg_init_size(fMsg.get(), size) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }
    }

    void Rebuild(size_t size, Alignment alignment) override
    {
        CloseMessage();
        fAlignment = alignment.alignment;
        fMsg = std::make_unique<zmq_msg_t>();

        if (fAlignment != 0) {
            auto ptrs = AllocateAligned(size, fAlignment);
            if (zmq_msg_init_data(fMsg.get(), ptrs.second, size, [](void* /* data */, void* hint) { free(hint); }, ptrs.first) != 0) {
                LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
            }
        } else {
            if (zmq_msg_init_size(fMsg.get(), size) != 0) {
                LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
            }
        }
    }

    void Rebuild(void* data, size_t size, fair::mq::FreeFn* ffn, void* hint = nullptr) override
    {
        CloseMessage();
        fMsg = std::make_unique<zmq_msg_t>();
        if (zmq_msg_init_data(fMsg.get(), data, size, ffn, hint) != 0) {
            LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
        }
    }

    void* GetData() const override
    {
        if (zmq_msg_size(fMsg.get()) > 0) {
            return zmq_msg_data(fMsg.get());
        } else {
            return nullptr;
        }
    }

    size_t GetSize() const override { return zmq_msg_size(fMsg.get()); }

    // To emulate shrinking, a new message is created with the new size (ViewMsg), that points to
    // the original buffer with the new size. Once the "view message" is transfered, the original is
    // destroyed. Used size is applied only once in ApplyUsedSize, which is called by the socket
    // before sending. This function just updates the desired size until the actual "resizing"
    // happens.
    bool SetUsedSize(size_t size, Alignment /* alignment */ = Alignment{0}) override
    {
        if (size == GetSize()) {
            // nothing to do
            return true;
        } else if (size > GetSize()) {
            LOG(error) << "cannot set used size higher than original.";
            return false;
        } else {
            auto newMsg = std::make_unique<zmq_msg_t>();
            void* data = GetData();
            if (zmq_msg_init_data(newMsg.get(), data, size, [](void* /* data */, void* obj) {
                    zmq_msg_close(static_cast<zmq_msg_t*>(obj));
                    delete static_cast<zmq_msg_t*>(obj);
                }, fMsg.get()) != 0) {
                LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
                return false;
            }
            fMsg.release();
            fMsg.swap(newMsg);
            return true;
        }
    }

    void Realign()
    {
        // if alignment is provided
        if (fAlignment != 0) {
            void* data = GetData();
            size_t size = GetSize();
            // if buffer is valid && not already aligned with the given alignment
            if (data != nullptr && reinterpret_cast<uintptr_t>(GetData()) % fAlignment) {
                // create new aligned buffer
                auto ptrs = AllocateAligned(size, fAlignment);
                std::memcpy(ptrs.second, zmq_msg_data(fMsg.get()), size);
                // rebuild the message with the new buffer
                Rebuild(ptrs.second, size, [](void* /* buf */, void* hint) { free(hint); }, ptrs.first);
            }
        }
    }

    Transport GetType() const override { return Transport::ZMQ; }

    void Copy(const fair::mq::Message& msg) override
    {
        const Message& zMsg = static_cast<const Message&>(msg);
        // Shares the message buffer between msg and this fMsg.
        if (zmq_msg_copy(fMsg.get(), zMsg.GetMessage()) != 0) {
            LOG(error) << "failed copying message, reason: " << zmq_strerror(errno);
            return;
        }
    }

    ~Message() override { CloseMessage(); }

  private:
    size_t fAlignment = 0;
    std::unique_ptr<zmq_msg_t> fMsg;

    zmq_msg_t* GetMessage() const { return fMsg.get(); }

    void CloseMessage()
    {
        if (zmq_msg_close(fMsg.get()) != 0) {
            LOG(error) << "failed closing message, reason: " << zmq_strerror(errno);
        }
        // reset the message object to allow reuse in Rebuild
        fMsg.reset(nullptr);
        fAlignment = 0;
    }
};

} // namespace fair::mq::zmq

#endif /* FAIR_MQ_ZMQ_MESSAGE_H */
