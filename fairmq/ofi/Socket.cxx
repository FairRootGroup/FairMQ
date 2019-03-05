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
#include <fairmq/Tools.h>
#include <FairMQLogger.h>

#include <asiofi.hpp>
#include <azmq/message.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/post.hpp>
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <thread>

#include <mutex>
#include <condition_variable>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

Socket::Socket(Context& context, const string& type, const string& name, const string& id /*= ""*/)
    : fContext(context)
    , fOfiInfo(nullptr)
    , fOfiFabric(nullptr)
    , fOfiDomain(nullptr)
    , fPassiveEndpoint(nullptr)
    , fDataEndpoint(nullptr)
    , fId(id + "." + name + "." + type)
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
    , fSndTimeout(100)
    , fRcvTimeout(100)
    , fRecvQueueWrite(fContext.GetIoContext(), ZMQ_PUSH)
    , fRecvQueueRead(fContext.GetIoContext(), ZMQ_PULL)
    , fSendSem(fContext.GetIoContext(), 300)
    , fRecvSem(fContext.GetIoContext(), 300)
    , fNeedOfiMemoryRegistration(false)
    , fBound(false)
    , fConnected(false)
{
    if (type != "pair") {
        throw SocketError{tools::ToString("Socket type '", type, "' not implemented for ofi transport.")};
    } else {
        // TODO wire this up with config
        azmq::socket::snd_hwm send_max(300);
        azmq::socket::rcv_hwm recv_max(300);
        fRecvQueueRead.set_option(send_max);
        fRecvQueueRead.set_option(recv_max);
        fRecvQueueWrite.set_option(send_max);
        fRecvQueueWrite.set_option(recv_max);

        // Setup internal queue
        auto hashed_id = hash<string>()(fId);
        auto queue_id = tools::ToString("inproc://TXQUEUE", hashed_id);
        queue_id = tools::ToString("inproc://RXQUEUE", hashed_id);
        LOG(debug) << "OFI transport (" << fId << "): " << "Binding RQR: " << queue_id;
        fRecvQueueRead.bind(queue_id);
        LOG(debug) << "OFI transport (" << fId << "): " << "Connecting RQW: " << queue_id;
        fRecvQueueWrite.connect(queue_id);
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
          fOfiInfo = tools::make_unique<asiofi::info>(addr.Ip.c_str(), to_string(addr.Port).c_str(), 0, hints);
      } else {
          fOfiInfo = tools::make_unique<asiofi::info>(addr.Ip.c_str(), to_string(addr.Port).c_str(), FI_SOURCE, hints);
      }

      LOG(debug) << "OFI transport: " << *fOfiInfo;

      fOfiFabric = tools::make_unique<asiofi::fabric>(*fOfiInfo);

      fOfiDomain = tools::make_unique<asiofi::domain>(*fOfiFabric);
    }
}

auto Socket::Bind(const string& addr) -> bool
try {
    fBound = false;
    fLocalAddr = Context::VerifyAddress(addr);
    if (fLocalAddr.Protocol == "verbs") {
        fNeedOfiMemoryRegistration = true;
    }

    InitOfi(fLocalAddr);

    fPassiveEndpoint = tools::make_unique<asiofi::passive_endpoint>(fContext.GetIoContext(), *fOfiFabric);
    //fPassiveEndpoint->set_local_address(Context::ConvertAddress(fLocalAddr));

    assert(!fDataEndpoint);

    fPassiveEndpoint->listen([&](asiofi::info&& info) {
        LOG(debug) << "OFI transport (" << fId << "): data band connection request received. Accepting ...";
        fDataEndpoint = tools::make_unique<asiofi::connected_endpoint>(fContext.GetIoContext(), *fOfiDomain, info);
        fDataEndpoint->enable();
        fDataEndpoint->accept([&]() {
            LOG(debug) << "OFI transport (" << fId << "): data band connection accepted.";

            boost::asio::post(fContext.GetIoContext(), bind(&Socket::RecvQueueReader, this));
            fBound = true;
        });
    });

    LOG(debug) << "OFI transport (" << fId << "): data band bound to " << fLocalAddr;

    while (!fBound) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    return true;
} catch (const SilentSocketError& e) {// TODO catch the correct ofi error
    // do not print error in this case, this is handled by FairMQDevice
    // in case no connection could be established after trying a number of random ports from a range.
    return false;
} catch (const SocketError& e) {
    LOG(error) << "OFI transport: " << e.what();
    return false;
}

auto Socket::Connect(const string& address) -> bool
try {
    fConnected = false;
    fRemoteAddr = Context::VerifyAddress(address);
    if (fRemoteAddr.Protocol == "verbs") {
        fNeedOfiMemoryRegistration = true;
    }

    InitOfi(fRemoteAddr);

    assert(!fDataEndpoint);

    fDataEndpoint = tools::make_unique<asiofi::connected_endpoint>(fContext.GetIoContext(), *fOfiDomain);
    fDataEndpoint->enable();

    LOG(debug) << "OFI transport (" << fId << "): Sending data band connection request to " << fRemoteAddr;

    fDataEndpoint->connect(Context::ConvertAddress(fRemoteAddr), [&](asiofi::eq::event event) {
        LOG(debug) << "OFI transport (" << fId << "): data band conn event happened";
        if (event == asiofi::eq::event::connected) {
            LOG(debug) << "OFI transport (" << fId << "): data band connected.";
            boost::asio::post(fContext.GetIoContext(), bind(&Socket::RecvQueueReader, this));
            fConnected = true;
        } else {
            LOG(error) << "Could not connect on the first try";
        }
    });

    while (!fConnected) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    return true;
} catch (const SilentSocketError& e) {
    // do not print error in this case, this is handled by FairMQDevice
    return false;
} catch (const exception& e) {
    LOG(error) << "OFI transport: " << e.what();
    return false;
}

auto Socket::Send(MessagePtr& msg, const int /*timeout*/) -> int
{
    // LOG(debug) << "OFI transport (" << fId << "): ENTER Send: data=" << msg->GetData() << ",size=" << msg->GetSize();

    try {
        fSendSem.wait();
        size_t size = msg->GetSize();
        OnSend(msg);
        return size;
    } catch (const exception& e) {
        LOG(error) << e.what();
        return -1;
    } catch (const boost::system::error_code& e) {
        LOG(error) << e;
        return -1;
    }
}

auto Socket::OnSend(MessagePtr& msg) -> void
{
    // LOG(debug) << "OFI transport (" << fId << "): ENTER OnSend";

    auto size = 2000000;

    // LOG(debug) << "OFI transport (" << fId << "):       OnSend: data=" << msg->GetData() << ",size=" << msg->GetSize();

    boost::asio::mutable_buffer buffer(msg->GetData(), size);

    if (fNeedOfiMemoryRegistration) {
        asiofi::memory_region mr(*fOfiDomain, buffer, asiofi::mr::access::send);
        auto desc = mr.desc();

        fDataEndpoint->send(buffer, desc, [&, size, msg2 = move(msg), mr2 = move(mr)](boost::asio::mutable_buffer) mutable {
            // LOG(debug) << "OFI transport (" << fId << "): >>>>> Data buffer sent";
            fBytesTx += size;
            fMessagesTx++;
            fSendSem.async_signal([&](const boost::system::error_code& ec) {
                if (!ec) {
                    // LOG(debug) << "OFI transport (" << fId << "):     > Signal fSendSem=" << fSendSem.get_value();
                }
            });
        });
    } else {
        fDataEndpoint->send(buffer, [&, size, msg2 = move(msg)](boost::asio::mutable_buffer) mutable {
            // LOG(debug) << "OFI transport (" << fId << "): >>>>> Data buffer sent";
            fBytesTx += size;
            fMessagesTx++;
            fSendSem.async_signal([&](const boost::system::error_code& ec) {
                if (!ec) {
                    // LOG(debug) << "OFI transport (" << fId << "):     > Signal fSendSem=" << fSendSem.get_value();
                }
            });
        });
    }

    // LOG(debug) << "OFI transport (" << fId << "): LEAVE OnSend";
}

auto Socket::Receive(MessagePtr& msg, const int /*timeout*/) -> int
try {
    // LOG(debug) << "OFI transport (" << fId << "): ENTER Receive";
    azmq::message zmsg;
    auto recv = fRecvQueueRead.receive(zmsg);

    size_t size = 0;
    if (recv > 0) {
        msg = move(*(static_cast<MessagePtr*>(zmsg.buffer().data())));
        size = msg->GetSize();
    }

    fBytesRx += size;
    fMessagesRx++;

    // LOG(debug) << "OFI transport (" << fId << "): LEAVE Receive";
    return size;
} catch (const exception& e) {
    LOG(error) << e.what();
    return -1;
} catch (const boost::system::error_code& e) {
    LOG(error) << e;
    return -1;
}

auto Socket::Receive(vector<MessagePtr>& msgVec, const int timeout) -> int64_t
{
    return ReceiveImpl(msgVec, 0, timeout);
}

auto Socket::RecvQueueReader() -> void
{
    fRecvSem.async_wait([&](const boost::system::error_code& ec) {
        if (!ec) {
            auto size = 2000000;

            auto msg = fContext.MakeReceiveMessage(size);
            boost::asio::mutable_buffer buffer(msg->GetData(), size);

            if (fNeedOfiMemoryRegistration) {
                asiofi::memory_region mr(*fOfiDomain, buffer, asiofi::mr::access::recv);
                auto desc = mr.desc();

                fDataEndpoint->recv(buffer, desc, [&, msg2 = move(msg), mr2 = move(mr)](boost::asio::mutable_buffer) mutable {
                    MessagePtr* msgptr(new std::unique_ptr<Message>(move(msg2)));
                    fRecvQueueWrite.async_send(azmq::message(boost::asio::const_buffer(msgptr, sizeof(MessagePtr))), [&](const boost::system::error_code& ec2, size_t /*bytes_transferred2*/) {
                        if (!ec2) {
                            // LOG(debug) << "OFI transport (" << fId << "): <<<<< Data buffer received, bytes_transferred2=" << bytes_transferred2;
                            fRecvSem.async_signal([&](const boost::system::error_code& ec3) {
                                if (!ec3) {
                                    // LOG(debug) << "OFI transport (" << fId << "):     < Signal fRecvSem";
                                }
                            });
                        }
                    });
                });
            } else {
                fDataEndpoint->recv(buffer, [&, msg2 = move(msg)](boost::asio::mutable_buffer) mutable {
                    MessagePtr* msgptr(new std::unique_ptr<Message>(move(msg2)));
                    fRecvQueueWrite.async_send(azmq::message(boost::asio::const_buffer(msgptr, sizeof(MessagePtr))), [&](const boost::system::error_code& ec2, size_t /*bytes_transferred2*/) {
                        if (!ec2) {
                            // LOG(debug) << "OFI transport (" << fId << "): <<<<< Data buffer received, bytes_transferred2=" << bytes_transferred2;
                            fRecvSem.async_signal([&](const boost::system::error_code& ec3) {
                                if (!ec3) {
                                    // LOG(debug) << "OFI transport (" << fId << "):     < Signal fRecvSem";
                                }
                            });
                        }
                    });
                });
            }

            boost::asio::post(fContext.GetIoContext(), bind(&Socket::RecvQueueReader, this));
        }
    });
}

auto Socket::Send(vector<MessagePtr>& msgVec, const int timeout) -> int64_t
{
    return SendImpl(msgVec, 0, timeout);
}

auto Socket::SendImpl(vector<FairMQMessagePtr>& /*msgVec*/, const int /*flags*/, const int /*timeout*/) -> int64_t 
{
    throw SocketError{"Not yet implemented."};
}

auto Socket::ReceiveImpl(vector<FairMQMessagePtr>& /*msgVec*/, const int /*flags*/, const int /*timeout*/) -> int64_t
{
    throw SocketError{"Not yet implemented."};
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
    // azmq::socket::linger opt(value);
    // fControlEndpoint.set_option(opt);
}

int Socket::GetLinger() const
{
    // azmq::socket::linger opt(0);
    // fControlEndpoint.get_option(opt);
    // return opt.value();
    return 0;
}

void Socket::SetSndBufSize(const int /*value*/)
{
    // azmq::socket::snd_hwm opt(value);
    // fControlEndpoint.set_option(opt);
}

int Socket::GetSndBufSize() const
{
    // azmq::socket::snd_hwm opt(0);
    // fControlEndpoint.get_option(opt);
    // return opt.value();
    return 0;
}

void Socket::SetRcvBufSize(const int /*value*/)
{
    // azmq::socket::rcv_hwm opt(value);
    // fControlEndpoint.set_option(opt);
}

int Socket::GetRcvBufSize() const
{
    // azmq::socket::rcv_hwm opt(0);
    // fControlEndpoint.get_option(opt);
    // return opt.value();
    return 0;
}

void Socket::SetSndKernelSize(const int /*value*/)
{
    // azmq::socket::snd_buf opt(value);
    // fControlEndpoint.set_option(opt);
}

int Socket::GetSndKernelSize() const
{
    // azmq::socket::snd_buf opt(0);
    // fControlEndpoint.get_option(opt);
    // return opt.value();
    return 0;
}

void Socket::SetRcvKernelSize(const int /*value*/)
{
    // azmq::socket::rcv_buf opt(value);
    // fControlEndpoint.set_option(opt);
}

int Socket::GetRcvKernelSize() const
{
    // azmq::socket::rcv_buf opt(0);
    // fControlEndpoint.get_option(opt);
    // return opt.value();
    return 0;
}

auto Socket::GetConstant(const string& constant) -> int
{
    if (constant == "")
        return 0;
    if (constant == "sub")
        return ZMQ_SUB;
    if (constant == "pub")
        return ZMQ_PUB;
    if (constant == "xsub")
        return ZMQ_XSUB;
    if (constant == "xpub")
        return ZMQ_XPUB;
    if (constant == "push")
        return ZMQ_PUSH;
    if (constant == "pull")
        return ZMQ_PULL;
    if (constant == "req")
        return ZMQ_REQ;
    if (constant == "rep")
        return ZMQ_REP;
    if (constant == "dealer")
        return ZMQ_DEALER;
    if (constant == "router")
        return ZMQ_ROUTER;
    if (constant == "pair")
        return ZMQ_PAIR;

    if (constant == "snd-hwm")
        return ZMQ_SNDHWM;
    if (constant == "rcv-hwm")
        return ZMQ_RCVHWM;
    if (constant == "snd-size")
        return ZMQ_SNDBUF;
    if (constant == "rcv-size")
        return ZMQ_RCVBUF;
    if (constant == "snd-more")
        return ZMQ_SNDMORE;
    if (constant == "rcv-more")
        return ZMQ_RCVMORE;

    if (constant == "linger")
        return ZMQ_LINGER;
    if (constant == "no-block")
        return ZMQ_DONTWAIT;
    if (constant == "snd-more no-block")
        return ZMQ_DONTWAIT|ZMQ_SNDMORE;

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

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */
