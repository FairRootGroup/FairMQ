/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/ControlMessages.h>
#include <fairmq/ofi/Socket.h>
#include <fairmq/ofi/TransportFactory.h>
#include <fairmq/tools/Strings.h>
#include <FairMQLogger.h>

#include <asiofi.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <sstream>
#include <thread>
#include <mutex>
#include <queue>

namespace fair::mq::ofi
{

using namespace std;

Socket::Socket(Context& context, const string& type, const string& name, const string& id /*= ""*/)
    : fContext(context)
    , fOfiInfo(nullptr)
    , fOfiFabric(nullptr)
    , fOfiDomain(nullptr)
    , fPassiveEndpoint(nullptr)
    , fDataEndpoint(nullptr)
    , fControlEndpoint(nullptr)
    , fId(id + "." + name + "." + type)
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
    , fSndTimeout(100)
    , fRcvTimeout(100)
    , fMultiPartRecvCounter(-1)
    , fSendPushSem(fContext.GetIoContext(), 384)
    , fSendPopSem(fContext.GetIoContext(), 0)
    , fRecvPushSem(fContext.GetIoContext(), 384)
    , fRecvPopSem(fContext.GetIoContext(), 0)
    , fNeedOfiMemoryRegistration(false)
{
    if (type != "pair") {
        throw SocketError{tools::ToString("Socket type '", type, "' not implemented for ofi transport.")};
    }
}

auto Socket::InitOfi(Address addr) -> void
{
    if (!fOfiInfo) {
      assert(!fOfiFabric);
      assert(!fOfiDomain);

      asiofi::hints hints;
      if (addr.Protocol == "tcp") {
          hints.set_provider("sockets");
      } else if (addr.Protocol == "verbs") {
          hints.set_provider("verbs");
      }
      if (fRemoteAddr == addr) {
          fOfiInfo = make_unique<asiofi::info>(addr.Ip.c_str(), std::to_string(addr.Port).c_str(), 0, hints);
      } else {
          fOfiInfo = make_unique<asiofi::info>(addr.Ip.c_str(), std::to_string(addr.Port).c_str(), FI_SOURCE, hints);
      }

      LOG(debug) << "OFI transport (" << fId << "): " << *fOfiInfo;

      fOfiFabric = make_unique<asiofi::fabric>(*fOfiInfo);

      fOfiDomain = make_unique<asiofi::domain>(*fOfiFabric);
    }
}

auto Socket::Bind(const string& addr) -> bool
try {
    fLocalAddr = Context::VerifyAddress(addr);
    if (fLocalAddr.Protocol == "verbs") {
        fNeedOfiMemoryRegistration = true;
    }

    InitOfi(fLocalAddr);

    fPassiveEndpoint = make_unique<asiofi::passive_endpoint>(fContext.GetIoContext(), *fOfiFabric);
    //fPassiveEndpoint->set_local_address(Context::ConvertAddress(fLocalAddr));

    BindControlEndpoint();

    return true;
}
// TODO catch the correct ofi error
catch (const SilentSocketError& e)
{
    // do not print error in this case, this is handled by FairMQDevice
    // in case no connection could be established after trying a number of random ports from a range.
    return false;
}
catch (const std::exception& e)
{
    LOG(error) << "OFI transport: " << e.what();
    return false;
}
catch (...)
{
    LOG(error) << "OFI transport: Unknown exception in ofi::Socket::Bind";
    return false;
}

auto Socket::BindControlEndpoint() -> void
{
    assert(!fControlEndpoint);

    fPassiveEndpoint->listen([&](asiofi::info&& info) {
        LOG(debug) << "OFI transport (" << fId
                   << "): control band connection request received. Accepting ...";
        fControlEndpoint = make_unique<asiofi::connected_endpoint>(
            fContext.GetIoContext(), *fOfiDomain, info);
        fControlEndpoint->enable();
        fControlEndpoint->accept([&]() {
            LOG(debug) << "OFI transport (" << fId << "): control band connection accepted.";

            BindDataEndpoint();
        });
    });

    LOG(debug) << "OFI transport (" << fId << "): control band bound to " << fLocalAddr;
}

auto Socket::BindDataEndpoint() -> void
{
    assert(!fDataEndpoint);

    fPassiveEndpoint->listen([&](asiofi::info&& info) {
        LOG(debug) << "OFI transport (" << fId
                   << "): data band connection request received. Accepting ...";
        fDataEndpoint = make_unique<asiofi::connected_endpoint>(
            fContext.GetIoContext(), *fOfiDomain, info);
        fDataEndpoint->enable();
        fDataEndpoint->accept([&]() {
            LOG(debug) << "OFI transport (" << fId << "): data band connection accepted.";

            if (fContext.GetSizeHint()) {
                boost::asio::post(fContext.GetIoContext(),
                                  std::bind(&Socket::SendQueueReaderStatic, this));
                boost::asio::post(fContext.GetIoContext(),
                                  std::bind(&Socket::RecvQueueReaderStatic, this));
            } else {
                boost::asio::post(fContext.GetIoContext(),
                                  std::bind(&Socket::SendQueueReader, this));
                boost::asio::post(fContext.GetIoContext(),
                                  std::bind(&Socket::RecvControlQueueReader, this));
            }
        });
    });

    LOG(debug) << "OFI transport (" << fId << "): data band bound to " << fLocalAddr;
}

auto Socket::Connect(const string& address) -> bool
try {
    fRemoteAddr = Context::VerifyAddress(address);
    if (fRemoteAddr.Protocol == "verbs") {
        fNeedOfiMemoryRegistration = true;
    }

    InitOfi(fRemoteAddr);

    ConnectEndpoint(fControlEndpoint, Band::Control);
    ConnectEndpoint(fDataEndpoint, Band::Data);

    if (fContext.GetSizeHint()) {
        boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::SendQueueReaderStatic, this));
        boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::RecvQueueReaderStatic, this));
    } else {
        boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::SendQueueReader, this));
        boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::RecvControlQueueReader, this));
    }

    return true;
}
catch (const SilentSocketError& e)
{
    // do not print error in this case, this is handled by FairMQDevice
    return false;
}
catch (const std::exception& e)
{
    LOG(error) << "OFI transport: " << e.what();
    return false;
}
catch (...)
{
    LOG(error) << "OFI transport: Unknown exception in ofi::Socket::Connect";
    return false;
}

auto Socket::ConnectEndpoint(std::unique_ptr<asiofi::connected_endpoint>& endpoint, Band type) -> void
{
    assert(!endpoint);

    std::string band(type == Band::Control ? "control" : "data");

    endpoint = make_unique<asiofi::connected_endpoint>(fContext.GetIoContext(), *fOfiDomain);
    endpoint->enable();

    LOG(debug) << "OFI transport (" << fId << "): Sending " << band << " band connection request to " << fRemoteAddr;

    std::mutex mtx;
    std::condition_variable cv;
    bool notified(false), connected(false);

    while (true) {
        endpoint->connect(Context::ConvertAddress(fRemoteAddr), [&, band](asiofi::eq::event event) {
            // LOG(debug) << "OFI transport (" << fId << "): " << band << " band conn event happened";
            std::unique_lock<std::mutex> lk2(mtx);
            notified = true;
            if (event == asiofi::eq::event::connected) {
                LOG(debug) << "OFI transport (" << fId << "): " << band << " band connected.";
                connected = true;
            } else {
                // LOG(debug) << "OFI transport (" << fId << "): " << band << " band connection refused. Trying again.";
            }
            lk2.unlock();
            cv.notify_one();
        });

        {
            std::unique_lock<std::mutex> lk(mtx);
            cv.wait(lk, [&] { return notified; });

            if (connected) {
                break;
            } else {
                notified = false;
                lk.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

auto Socket::Send(MessagePtr& msg, const int /*timeout*/) -> int64_t
{
    // timeout argument not yet implemented

    std::vector<MessagePtr> msgVec;
    msgVec.reserve(1);
    msgVec.emplace_back(std::move(msg));

    return Send(msgVec);
}

auto Socket::Send(std::vector<MessagePtr>& msgVec, const int /*timeout*/) -> int64_t
try {
    // timeout argument not yet implemented

    int size(0);
    for (auto& msg : msgVec) {
        size += msg->GetSize();
    }

    fSendPushSem.wait();
    {
        std::lock_guard<std::mutex> lk(fSendQueueMutex);
        fSendQueue.emplace(std::move(msgVec));
    }
    fSendPopSem.signal();

    return size;
} catch (const std::exception& e) {
    LOG(error) << e.what();
    return static_cast<int64_t>(TransferCode::error);
}

auto Socket::SendQueueReader() -> void
{
    fSendPopSem.async_wait([&] {
        // Read msg from send queue
        std::unique_lock<std::mutex> lk(fSendQueueMutex);
        std::vector<MessagePtr> msgVec(std::move(fSendQueue.front()));
        fSendQueue.pop();
        lk.unlock();

        bool postMultiPartStartBuffer = msgVec.size() > 1;
        for (auto& msg : msgVec) {
            // Create control message
            ofi::unique_ptr<ControlMessage> ctrl(nullptr);
            if (postMultiPartStartBuffer) {
                postMultiPartStartBuffer = false;
                ctrl = MakeControlMessageWithPmr<PostMultiPartStartBuffer>(fControlMemPool);
                ctrl->msg.postMultiPartStartBuffer.numParts = msgVec.size();
                ctrl->msg.postMultiPartStartBuffer.size = msg->GetSize();
            } else {
                ctrl = MakeControlMessageWithPmr<PostBuffer>(fControlMemPool);
                ctrl->msg.postBuffer.size = msg->GetSize();
            }

            // Send control message
            boost::asio::mutable_buffer ctrlMsg(ctrl.get(), sizeof(ControlMessage));

            if (fNeedOfiMemoryRegistration) {
                asiofi::memory_region mr(*fOfiDomain, ctrlMsg, asiofi::mr::access::send);
                auto desc = mr.desc();
                fControlEndpoint->send(ctrlMsg,
                                       desc,
                                       [&, ctrl2 = std::move(ctrlMsg), mr2 = std::move(mr)](
                                           boost::asio::mutable_buffer) mutable {});
            } else {
                fControlEndpoint->send(
                    ctrlMsg, [&, ctrl2 = std::move(ctrl)](boost::asio::mutable_buffer) mutable {});
            }

            // Send data message
            const auto size = msg->GetSize();

            if (size) {
                boost::asio::mutable_buffer buffer(msg->GetData(), size);

                if (fNeedOfiMemoryRegistration) {
                    asiofi::memory_region mr(*fOfiDomain, buffer, asiofi::mr::access::send);
                    auto desc = mr.desc();

                    fDataEndpoint->send(buffer,
                                        desc,
                                        [&, size, msg2 = std::move(msg), mr2 = std::move(mr)](
                                            boost::asio::mutable_buffer) mutable {
                                            fBytesTx += size;
                                            fMessagesTx++;
                                            fSendPushSem.signal();
                                        });
                } else {
                    fDataEndpoint->send(
                        buffer, [&, size, msg2 = std::move(msg)](boost::asio::mutable_buffer) mutable {
                            fBytesTx += size;
                            fMessagesTx++;
                            fSendPushSem.signal();
                        });
                }
            } else {
                ++fMessagesTx;
                fSendPushSem.signal();
            }
        }

        boost::asio::dispatch(fContext.GetIoContext(), std::bind(&Socket::SendQueueReader, this));
    });
}

auto Socket::SendQueueReaderStatic() -> void
{
    fSendPopSem.async_wait([&] {
        // Read msg from send queue
        std::unique_lock<std::mutex> lk(fSendQueueMutex);
        std::vector<MessagePtr> msgVec(std::move(fSendQueue.front()));
        fSendQueue.pop();
        lk.unlock();

        bool postMultiPartStartBuffer = msgVec.size() > 1;
        if (postMultiPartStartBuffer) {
            throw SocketError{tools::ToString("Multipart API not supported in static size mode.")};
        }

        MessagePtr& msg = msgVec[0];

        // Send data message
        const auto size = msg->GetSize();

        if (size) {
            boost::asio::mutable_buffer buffer(msg->GetData(), size);

            if (fNeedOfiMemoryRegistration) {
                asiofi::memory_region mr(*fOfiDomain, buffer, asiofi::mr::access::send);
                auto desc = mr.desc();

                fDataEndpoint->send(buffer,
                                    desc,
                                    [&, size, msg2 = std::move(msg), mr2 = std::move(mr)](
                                        boost::asio::mutable_buffer) mutable {
                                        fBytesTx += size;
                                        fMessagesTx++;
                                        fSendPushSem.signal();
                                    });
            } else {
                fDataEndpoint->send(
                    buffer, [&, size, msg2 = std::move(msg)](boost::asio::mutable_buffer) mutable {
                        fBytesTx += size;
                        fMessagesTx++;
                        fSendPushSem.signal();
                    });
            }
        } else {
            ++fMessagesTx;
            fSendPushSem.signal();
        }

        boost::asio::dispatch(fContext.GetIoContext(), std::bind(&Socket::SendQueueReaderStatic, this));
    });
}

auto Socket::Receive(MessagePtr& msg, const int /*timeout*/) -> int64_t
try {
    // timeout argument not yet implemented

    fRecvPopSem.wait();
    {
        std::lock_guard<std::mutex> lk(fRecvQueueMutex);
        msg = std::move(fRecvQueue.front().front());
        fRecvQueue.pop();
    }
    fRecvPushSem.signal();

    int size(msg->GetSize());
    fBytesRx += size;
    ++fMessagesRx;

    return size;
} catch (const std::exception& e) {
    LOG(error) << e.what();
    return static_cast<int>(TransferCode::error);
}

auto Socket::Receive(std::vector<MessagePtr>& msgVec, const int /*timeout*/) -> int64_t
try {
    // timeout argument not yet implemented

    fRecvPopSem.wait();
    {
        std::lock_guard<std::mutex> lk(fRecvQueueMutex);
        msgVec = std::move(fRecvQueue.front());
        fRecvQueue.pop();
    }
    fRecvPushSem.signal();

    int64_t size(0);
    for (auto& msg : msgVec) {
        size += msg->GetSize();
    }
    fBytesRx += size;
    ++fMessagesRx;

    return size;
} catch (const std::exception& e) {
    LOG(error) << e.what();
    return static_cast<int64_t>(TransferCode::error);
}

auto Socket::RecvControlQueueReader() -> void
{
    fRecvPushSem.async_wait([&] {
        // Receive control message
        ofi::unique_ptr<ControlMessage> ctrl(MakeControlMessageWithPmr<Empty>(fControlMemPool));
        boost::asio::mutable_buffer ctrlMsg(ctrl.get(), sizeof(ControlMessage));

        if (fNeedOfiMemoryRegistration) {
            asiofi::memory_region mr(*fOfiDomain, ctrlMsg, asiofi::mr::access::recv);
            auto desc = mr.desc();

            fControlEndpoint->recv(
                ctrlMsg,
                desc,
                [&, ctrl2 = std::move(ctrl), mr2 = std::move(mr)](
                    boost::asio::mutable_buffer) mutable { OnRecvControl(std::move(ctrl2)); });
        } else {
            fControlEndpoint->recv(
                ctrlMsg, [&, ctrl2 = std::move(ctrl)](boost::asio::mutable_buffer) mutable {
                    OnRecvControl(std::move(ctrl2));
                });
        }
    });
}

auto Socket::OnRecvControl(ofi::unique_ptr<ControlMessage> ctrl) -> void
{
    // Check control message type
    auto size(0);
    if (ctrl->type == ControlMessageType::PostMultiPartStartBuffer) {
        size = ctrl->msg.postMultiPartStartBuffer.size;
        if (fMultiPartRecvCounter == -1) {
            fMultiPartRecvCounter = ctrl->msg.postMultiPartStartBuffer.numParts;
            assert(fInflightMultiPartMessage.empty());
            fInflightMultiPartMessage.reserve(ctrl->msg.postMultiPartStartBuffer.numParts);
        } else {
            throw SocketError{tools::ToString(
                "OFI transport: Received control start of new multi part message without completed "
                "reception of previous multi part message. Number of parts missing: ",
                fMultiPartRecvCounter)};
        }
    } else if (ctrl->type == ControlMessageType::PostBuffer) {
        size = ctrl->msg.postBuffer.size;
    } else {
        throw SocketError{tools::ToString("OFI transport: Unknown control message type: '",
                                          static_cast<int>(ctrl->type))};
    }

    // Receive data
    auto msg = fContext.MakeReceiveMessage(size);

    if (size) {
        boost::asio::mutable_buffer buffer(msg->GetData(), size);

        if (fNeedOfiMemoryRegistration) {
            asiofi::memory_region mr(*fOfiDomain, buffer, asiofi::mr::access::recv);
            auto desc = mr.desc();

            fDataEndpoint->recv(
                buffer,
                desc,
                [&, msg2 = std::move(msg), mr2 = std::move(mr)](
                    boost::asio::mutable_buffer) mutable { DataMessageReceived(std::move(msg2)); });

        } else {
            fDataEndpoint->recv(buffer,
                                [&, msg2 = std::move(msg)](boost::asio::mutable_buffer) mutable {
                                    DataMessageReceived(std::move(msg2));
                                });
        }
    } else {
        DataMessageReceived(std::move(msg));
    }

    boost::asio::dispatch(fContext.GetIoContext(),
                          std::bind(&Socket::RecvControlQueueReader, this));
}

auto Socket::RecvQueueReaderStatic() -> void
{
    fRecvPushSem.async_wait([&] {
        static size_t size = fContext.GetSizeHint();
        // Receive data
        auto msg = fContext.MakeReceiveMessage(size);

        if (size) {
            boost::asio::mutable_buffer buffer(msg->GetData(), size);

            if (fNeedOfiMemoryRegistration) {
                asiofi::memory_region mr(*fOfiDomain, buffer, asiofi::mr::access::recv);
                auto desc = mr.desc();

                fDataEndpoint->recv(buffer,
                                    desc,
                                    [&, msg2 = std::move(msg), mr2 = std::move(mr)](
                                        boost::asio::mutable_buffer) mutable {
                                        DataMessageReceived(std::move(msg2));
                                    });

            } else {
                fDataEndpoint->recv(
                    buffer, [&, msg2 = std::move(msg)](boost::asio::mutable_buffer) mutable {
                        DataMessageReceived(std::move(msg2));
                    });
            }
        } else {
            DataMessageReceived(std::move(msg));
        }

        boost::asio::dispatch(fContext.GetIoContext(),
                              std::bind(&Socket::RecvQueueReaderStatic, this));
    });
}

auto Socket::DataMessageReceived(MessagePtr msg) -> void
{
    if (fMultiPartRecvCounter > 0) {
        --fMultiPartRecvCounter;
        fInflightMultiPartMessage.push_back(std::move(msg));
    }

    if (fMultiPartRecvCounter == 0) {
        std::unique_lock<std::mutex> lk(fRecvQueueMutex);
        fRecvQueue.push(std::move(fInflightMultiPartMessage));
        lk.unlock();
        fMultiPartRecvCounter = -1;
        fRecvPopSem.signal();
    } else if (fMultiPartRecvCounter == -1) {
        std::vector<MessagePtr> msgVec;
        msgVec.push_back(std::move(msg));
        std::unique_lock<std::mutex> lk(fRecvQueueMutex);
        fRecvQueue.push(std::move(msgVec));
        lk.unlock();
        fRecvPopSem.signal();
    }

}

auto Socket::Close() -> void {}

auto Socket::SetOption(const string& /*option*/, const void* /*value*/, size_t /*valueSize*/) -> void
{
    // if (zmq_setsockopt(fControlSocket, GetConstant(option), value, valueSize) < 0) {
        // throw SocketError{tools::ToString("Failed setting socket option, reason: ", zmq_strerror(errno))};
    // }
}

auto Socket::GetOption(const string& /*option*/, void* /*value*/, size_t* /*valueSize*/) -> void
{
    // if (zmq_getsockopt(fControlSocket, GetConstant(option), value, valueSize) < 0) {
        // throw SocketError{tools::ToString("Failed getting socket option, reason: ", zmq_strerror(errno))};
    // }
}

void Socket::SetLinger(const int /*value*/)
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
}

int Socket::GetLinger() const
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
    return 0;
}

void Socket::SetSndBufSize(const int /*value*/)
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
}

int Socket::GetSndBufSize() const
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
    return 0;
}

void Socket::SetRcvBufSize(const int /*value*/)
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
}

int Socket::GetRcvBufSize() const
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
    return 0;
}

void Socket::SetSndKernelSize(const int /*value*/)
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
}

int Socket::GetSndKernelSize() const
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
    return 0;
}

void Socket::SetRcvKernelSize(const int /*value*/)
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
}

int Socket::GetRcvKernelSize() const
{
    LOG(debug) << "OFI transport (" << fId << "): Not yet implemented.";
    return 0;
}

auto Socket::GetConstant(const string& /*constant*/) -> int
{
    LOG(debug) << "OFI transport: Not yet implemented.";
    return -1;
}

Socket::~Socket()
{
    try {
        Close(); // NOLINT(clang-analyzer-optin.cplusplus.VirtualCall)
    } catch (SocketError& e) {
        LOG(error) << e.what();
    }
}

} // namespace fair::mq::ofi
