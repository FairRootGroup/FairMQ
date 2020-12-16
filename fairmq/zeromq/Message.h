/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_ZMQ_MESSAGE_H
#define FAIR_MQ_ZMQ_MESSAGE_H

#include <fairmq/Tools.h>
#include <fairmq/zeromq/UnmanagedRegion.h>
#include <FairMQLogger.h>
#include <FairMQMessage.h>
#include <FairMQUnmanagedRegion.h>

#include <zmq.h>

#include <cstddef>
#include <cstdlib> // malloc, aligned_alloc
#include <cstring>
#include <memory>
#include <string>

namespace fair
{
namespace mq
{
namespace zmq
{

class Socket;

class Message final : public fair::mq::Message
{
    friend class Socket;

  public:
    Message(FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fAlignment(0)
        , fMsg(tools::make_unique<zmq_msg_t>())
    {
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }
    Message(Alignment alignment, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fAlignment(alignment.alignment)
        , fMsg(tools::make_unique<zmq_msg_t>())
    {
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }

    Message(const size_t size, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fAlignment(0)
        , fMsg(tools::make_unique<zmq_msg_t>())
    {
        void* ptr = malloc(size);
        if (!ptr) {
            LOG(error) << "failed to allocate buffer with provided size (" << size << ").";
        }
        if (zmq_msg_init_data(fMsg.get(), ptr, size, [](void* data, void*) { free(data); }, nullptr) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }
    }

    Message(const size_t size, Alignment alignment, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fAlignment(alignment.alignment)
        , fMsg(tools::make_unique<zmq_msg_t>())
    {
        void* ptr = nullptr;
        if (alignment.alignment != 0) {
            ptr = aligned_alloc(size, fAlignment);
        } else {
            ptr = malloc(size);
        }
        if (!ptr) {
            LOG(error) << "failed to allocate buffer with provided size (" << size << ") and alignment (" << alignment.alignment << ").";
        }
        if (zmq_msg_init_data(fMsg.get(), ptr, size, [](void* data, void*) { free(data); }, nullptr) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }
    }

    Message(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fAlignment(0)
        , fMsg(tools::make_unique<zmq_msg_t>())
    {
        if (zmq_msg_init_data(fMsg.get(), data, size, ffn, hint) != 0) {
            LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
        }
    }

    Message(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fAlignment(0)
        , fMsg(tools::make_unique<zmq_msg_t>())
    {
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
        fMsg = tools::make_unique<zmq_msg_t>();
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }

    void Rebuild(const size_t size) override
    {
        CloseMessage();
        fMsg = tools::make_unique<zmq_msg_t>();
        if (zmq_msg_init_size(fMsg.get(), size) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }
    }

    void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override
    {
        CloseMessage();
        fMsg = tools::make_unique<zmq_msg_t>();
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
    bool SetUsedSize(const size_t size) override
    {
        if (size == GetSize()) {
            // nothing to do
            return true;
        } else if (size > GetSize()) {
            LOG(error) << "cannot set used size higher than original.";
            return false;
        } else {
            auto newMsg = tools::make_unique<zmq_msg_t>();
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

    // void Realign()
    // {
    //     if (fAlignment != 0) {
    //         if (reinterpret_cast<uintptr_t>(GetData()) % fAlignment) {
    //         }
    //     }
    // }

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
    size_t fAlignment;
    std::unique_ptr<zmq_msg_t> fMsg;

    zmq_msg_t* GetMessage() const { return fMsg.get(); }

    void CloseMessage()
    {
        if (zmq_msg_close(fMsg.get()) != 0) {
            LOG(error) << "failed closing message, reason: " << zmq_strerror(errno);
        }
        // reset the message object to allow reuse in Rebuild
        fMsg.reset(nullptr);
    }
};

}   // namespace zmq
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_ZMQ_MESSAGE_H */
