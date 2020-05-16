/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQMESSAGEZMQ_H_
#define FAIRMQMESSAGEZMQ_H_

#include <fairmq/Tools.h>
#include "FairMQUnmanagedRegionZMQ.h"
#include <FairMQLogger.h>
#include <FairMQMessage.h>
#include <FairMQUnmanagedRegion.h>

#include <zmq.h>

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>

class FairMQTransportFactory;

class FairMQSocketZMQ;

class FairMQMessageZMQ final : public FairMQMessage
{
    friend class FairMQSocketZMQ;

  public:
    FairMQMessageZMQ(FairMQTransportFactory* factory = nullptr)
        : FairMQMessage(factory)
        , fUsedSizeModified(false)
        , fUsedSize()
        , fMsg(fair::mq::tools::make_unique<zmq_msg_t>())
        , fViewMsg(nullptr)
    {
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }

    FairMQMessageZMQ(const size_t size, FairMQTransportFactory* factory = nullptr)
        : FairMQMessage(factory)
        , fUsedSizeModified(false)
        , fUsedSize(size)
        , fMsg(fair::mq::tools::make_unique<zmq_msg_t>())
        , fViewMsg(nullptr)
    {
        if (zmq_msg_init_size(fMsg.get(), size) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }
    }

    FairMQMessageZMQ(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr, FairMQTransportFactory* factory = nullptr)
        : FairMQMessage(factory)
        , fUsedSizeModified(false)
        , fUsedSize()
        , fMsg(fair::mq::tools::make_unique<zmq_msg_t>())
        , fViewMsg(nullptr)
    {
        if (zmq_msg_init_data(fMsg.get(), data, size, ffn, hint) != 0) {
            LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
        }
    }

    FairMQMessageZMQ(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0, FairMQTransportFactory* factory = nullptr)
        : FairMQMessage(factory)
        , fUsedSizeModified(false)
        , fUsedSize()
        , fMsg(fair::mq::tools::make_unique<zmq_msg_t>())
        , fViewMsg(nullptr)
    {
        // FIXME: make this zero-copy:
        // simply taking over the provided buffer can casue premature delete, since region could be
        // destroyed before the message is sent out. Needs lifetime extension for the ZMQ region.
        if (zmq_msg_init_size(fMsg.get(), size) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }

        std::memcpy(zmq_msg_data(fMsg.get()), data, size);
        // call region callback
        auto ptr = static_cast<FairMQUnmanagedRegionZMQ*>(region.get());
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
        fMsg = fair::mq::tools::make_unique<zmq_msg_t>();
        if (zmq_msg_init(fMsg.get()) != 0) {
            LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
        }
    }

    void Rebuild(const size_t size) override
    {
        CloseMessage();
        fMsg = fair::mq::tools::make_unique<zmq_msg_t>();
        if (zmq_msg_init_size(fMsg.get(), size) != 0) {
            LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
        }
    }

    void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override
    {
        CloseMessage();
        fMsg = fair::mq::tools::make_unique<zmq_msg_t>();
        if (zmq_msg_init_data(fMsg.get(), data, size, ffn, hint) != 0) {
            LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
        }
    }

    void* GetData() const override
    {
        if (!fViewMsg) {
            return zmq_msg_data(fMsg.get());
        } else {
            return zmq_msg_data(fViewMsg.get());
        }
    }

    size_t GetSize() const override
    {
        if (fUsedSizeModified) {
            return fUsedSize;
        } else {
            return zmq_msg_size(fMsg.get());
        }
    }

    // To emulate shrinking, a new message is created with the new size (ViewMsg), that points to
    // the original buffer with the new size. Once the "view message" is transfered, the original is
    // destroyed. Used size is applied only once in ApplyUsedSize, which is called by the socket
    // before sending. This function just updates the desired size until the actual "resizing"
    // happens.
    bool SetUsedSize(const size_t size) override
    {
        if (size <= zmq_msg_size(fMsg.get())) {
            fUsedSize = size;
            fUsedSizeModified = true;
            return true;
        } else {
            LOG(error) << "cannot set used size higher than original.";
            return false;
        }
    }

    void ApplyUsedSize()
    {
        // Apply only once (before actual send).
        // The check is needed because a send could fail and can be reattempted by the user, in this
        // case we do not want to modify buffer again.
        if (fUsedSizeModified && !fViewMsg) {
            fViewMsg = fair::mq::tools::make_unique<zmq_msg_t>();
            void* ptr = zmq_msg_data(fMsg.get());
            if (zmq_msg_init_data(fViewMsg.get(), ptr, fUsedSize, [](void* /* data */, void* obj) {
                    zmq_msg_close(static_cast<zmq_msg_t*>(obj));
                    delete static_cast<zmq_msg_t*>(obj);
                }, fMsg.release()) != 0) {
                LOG(error) << "failed initializing view message, reason: " << zmq_strerror(errno);
            }
        }
    }

    fair::mq::Transport GetType() const override { return fair::mq::Transport::ZMQ; }

    void Copy(const FairMQMessage& msg) override
    {
        const FairMQMessageZMQ& zMsg = static_cast<const FairMQMessageZMQ&>(msg);
        // Shares the message buffer between msg and this fMsg.
        if (zmq_msg_copy(fMsg.get(), zMsg.GetMessage()) != 0) {
            LOG(error) << "failed copying message, reason: " << zmq_strerror(errno);
            return;
        }

        // if the target message has been resized, apply same to this message also
        if (zMsg.fUsedSizeModified) {
            fUsedSizeModified = true;
            fUsedSize = zMsg.fUsedSize;
        }
    }

    ~FairMQMessageZMQ() override { CloseMessage(); }

  private:
    bool fUsedSizeModified;
    size_t fUsedSize;
    std::unique_ptr<zmq_msg_t> fMsg;
    std::unique_ptr<zmq_msg_t> fViewMsg;   // view on a subset of fMsg (treating it as user buffer)

    zmq_msg_t* GetMessage() const
    {
        if (!fViewMsg) {
            return fMsg.get();
        } else {
            return fViewMsg.get();
        }
    }

    void CloseMessage()
    {
        if (!fViewMsg) {
            if (zmq_msg_close(fMsg.get()) != 0) {
                LOG(error) << "failed closing message, reason: " << zmq_strerror(errno);
            }
            // reset the message object to allow reuse in Rebuild
            fMsg.reset(nullptr);
        } else {
            if (zmq_msg_close(fViewMsg.get()) != 0) {
                LOG(error) << "failed closing message, reason: " << zmq_strerror(errno);
            }
            // reset the message object to allow reuse in Rebuild
            fViewMsg.reset(nullptr);
        }
        fUsedSizeModified = false;
        fUsedSize = 0;
    }
};

#endif /* FAIRMQMESSAGEZMQ_H_ */
