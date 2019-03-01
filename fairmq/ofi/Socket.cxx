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
    , fControlEndpoint(nullptr)
    , fId(id + "." + name + "." + type)
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
    , fSndTimeout(100)
    , fRcvTimeout(100)
    , fSendQueueWrite(fContext.GetIoContext(), ZMQ_PUSH)
    , fSendQueueRead(fContext.GetIoContext(), ZMQ_PULL)
    , fRecvQueueWrite(fContext.GetIoContext(), ZMQ_PUSH)
    , fRecvQueueRead(fContext.GetIoContext(), ZMQ_PULL)
    , fSendSem(fContext.GetIoContext(), 300)
    , fRecvSem(fContext.GetIoContext(), 300)
    , fNeedOfiMemoryRegistration(false)
{
    if (type != "pair") {
        throw SocketError{tools::ToString("Socket type '", type, "' not implemented for ofi transport.")};
    } else {
        // TODO wire this up with config
        azmq::socket::snd_hwm send_max(300);
        azmq::socket::rcv_hwm recv_max(300);
        fSendQueueRead.set_option(send_max);
        fSendQueueRead.set_option(recv_max);
        fSendQueueWrite.set_option(send_max);
        fSendQueueWrite.set_option(recv_max);
        fRecvQueueRead.set_option(send_max);
        fRecvQueueRead.set_option(recv_max);
        fRecvQueueWrite.set_option(send_max);
        fRecvQueueWrite.set_option(recv_max);

        // Setup internal queue
        auto hashed_id = std::hash<std::string>()(fId);
        auto queue_id = tools::ToString("inproc://TXQUEUE", hashed_id);
        LOG(debug) << "OFI transport (" << fId << "): " << "Binding SQR: " << queue_id;
        fSendQueueRead.bind(queue_id);
        LOG(debug) << "OFI transport (" << fId << "): " << "Connecting SQW: " << queue_id;
        fSendQueueWrite.connect(queue_id);
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
          fOfiInfo = tools::make_unique<asiofi::info>(addr.Ip.c_str(), std::to_string(addr.Port).c_str(), 0, hints);
      } else {
          fOfiInfo = tools::make_unique<asiofi::info>(addr.Ip.c_str(), std::to_string(addr.Port).c_str(), FI_SOURCE, hints);
      }

      LOG(debug) << "OFI transport: " << *fOfiInfo;

      fOfiFabric = tools::make_unique<asiofi::fabric>(*fOfiInfo);

      fOfiDomain = tools::make_unique<asiofi::domain>(*fOfiFabric);
    }
}

auto Socket::Bind(const string& addr) -> bool
try {
    fLocalAddr = Context::VerifyAddress(addr);
    if (fLocalAddr.Protocol == "verbs") {
        fNeedOfiMemoryRegistration = true;
    }

    InitOfi(fLocalAddr);

    fPassiveEndpoint = tools::make_unique<asiofi::passive_endpoint>(fContext.GetIoContext(), *fOfiFabric);
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
catch (const SocketError& e)
{
    LOG(error) << "OFI transport: " << e.what();
    return false;
}

auto Socket::BindControlEndpoint() -> void
{
    assert(!fControlEndpoint);

    fPassiveEndpoint->listen([&](asiofi::info&& info) {
        LOG(debug) << "OFI transport (" << fId
                   << "): control band connection request received. Accepting ...";
        fControlEndpoint = tools::make_unique<asiofi::connected_endpoint>(
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
        fDataEndpoint = tools::make_unique<asiofi::connected_endpoint>(
            fContext.GetIoContext(), *fOfiDomain, info);
        fDataEndpoint->enable();
        fDataEndpoint->accept([&]() {
            LOG(debug) << "OFI transport (" << fId << "): data band connection accepted.";

            boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::SendQueueReader, this));
            boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::RecvControlQueueReader, this));
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

auto Socket::ConnectEndpoint(std::unique_ptr<asiofi::connected_endpoint>& endpoint, Band type) -> void
{
    assert(!endpoint);

    std::string band(type == Band::Control ? "control" : "data");

    endpoint = tools::make_unique<asiofi::connected_endpoint>(fContext.GetIoContext(), *fOfiDomain);
    endpoint->enable();

    LOG(debug) << "OFI transport (" << fId << "): Sending " << band << " band connection request to " << fRemoteAddr;

    endpoint->connect(Context::ConvertAddress(fRemoteAddr), [&, band, type](asiofi::eq::event event) {
        LOG(debug) << "OFI transport (" << fId << "): " << band << " band conn event happened";
        if (event == asiofi::eq::event::connected) {
            LOG(debug) << "OFI transport (" << fId << "): " << band << " band connected.";
            if (type == Band::Control) {
                ConnectEndpoint(fDataEndpoint, Band::Data);
            } else {
                boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::SendQueueReader, this));
                boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::RecvControlQueueReader, this));
            }
        } else {
            LOG(error) << "asdf";
        }
    });
}

// auto Socket::ReceiveDataAddressAnnouncement() -> void
// {
    // azmq::message ctrl;
    // auto recv = fControlEndpoint.receive(ctrl);
    // assert(recv == sizeof(DataAddressAnnouncement)); (void)recv;
    // auto daa(static_cast<const DataAddressAnnouncement*>(ctrl.data()));
    // assert(daa->type == ControlMessageType::DataAddressAnnouncement);
//
    // sockaddr_in remoteAddr;
    // remoteAddr.sin_family = AF_INET;
    // remoteAddr.sin_port = daa->port;
    // remoteAddr.sin_addr.s_addr = daa->ipv4;
//
    // auto addr = Context::ConvertAddress(remoteAddr);
    // addr.Protocol = fRemoteDataAddr.Protocol;
    // LOG(debug) << "OFI transport (" << fId << "): Data address announcement of remote endpoint received: " << addr;
    // fRemoteDataAddr = addr;
// }
//
// auto Socket::AnnounceDataAddress() -> void
// {
    // fLocalDataAddr = fDataEndpoint->get_local_address();
    // LOG(debug) << "Address of local ofi endpoint in socket " << fId << ": " << Context::ConvertAddress(fLocalDataAddr);
//
    // Create new data address announcement message
    // auto daa = MakeControlMessage<DataAddressAnnouncement>();
    // auto addr = Context::ConvertAddress(fLocalDataAddr);
    // daa.ipv4 = addr.sin_addr.s_addr;
    // daa.port = addr.sin_port;
//
    // auto sent = fControlEndpoint.send(boost::asio::buffer(daa));
    // assert(sent == sizeof(addr)); (void)sent;
//
    // LOG(debug) << "OFI transport (" << fId << "): data band address " << fLocalDataAddr << " announced.";
// }

auto Socket::Send(MessagePtr& msg, const int /*timeout*/) -> int
{
    // LOG(debug) << "OFI transport (" << fId << "): ENTER Send: data=" << msg->GetData() << ",size=" << msg->GetSize();

    MessagePtr* msgptr(new std::unique_ptr<Message>(std::move(msg)));
    try {
        auto res = fSendQueueWrite.send(boost::asio::const_buffer(msgptr, sizeof(MessagePtr)), 0);

        // LOG(debug) << "OFI transport (" << fId << "): LEAVE Send";
        return res;
    } catch (const std::exception& e) {
        msg = std::move(*msgptr);
        LOG(error) << e.what();
        return -1;
    } catch (const boost::system::error_code& e) {
        msg = std::move(*msgptr);
        LOG(error) << e;
        return -1;
    }
}

auto Socket::Receive(MessagePtr& msg, const int /*timeout*/) -> int
{
    // LOG(debug) << "OFI transport (" << fId << "): ENTER Receive";

    try {
        azmq::message zmsg;
        auto recv = fRecvQueueRead.receive(zmsg);

        size_t size(0);
        if (recv > 0) {
            msg = std::move(*(static_cast<MessagePtr*>(zmsg.buffer().data())));
            size = msg->GetSize();
        }

        fBytesRx += size;
        fMessagesRx++;

        // LOG(debug) << "OFI transport (" << fId << "): LEAVE Receive";
        return size;
    } catch (const std::exception& e) {
        LOG(error) << e.what();
        return -1;
    } catch (const boost::system::error_code& e) {
        LOG(error) << e;
        return -1;
    }
}

auto Socket::Send(std::vector<MessagePtr>& msgVec, const int timeout) -> int64_t
{
    return SendImpl(msgVec, 0, timeout);
}

auto Socket::Receive(std::vector<MessagePtr>& msgVec, const int timeout) -> int64_t
{
    return ReceiveImpl(msgVec, 0, timeout);
}

auto Socket::SendQueueReader() -> void
{
    fSendSem.async_wait([&](const boost::system::error_code& ec) {
        if (!ec) {
            // LOG(debug) << "OFI transport (" << fId << "):     < Wait fSendSem=" <<
            // fSendSem.get_value();
            fSendQueueRead.async_receive([&](const boost::system::error_code& ec2,
                                             azmq::message& zmsg,
                                             size_t bytes_transferred) {
                if (!ec2) {
                    OnSend(zmsg, bytes_transferred);
                }
            });
        }
    });
}

auto Socket::OnSend(azmq::message& zmsg, size_t /*bytes_transferred*/) -> void
{
    // LOG(debug) << "OFI transport (" << fId << "): ENTER OnSend: bytes_transferred=" << bytes_transferred;

    MessagePtr msg(std::move(*(static_cast<MessagePtr*>(zmsg.buffer().data()))));
    auto size = msg->GetSize();

    // LOG(debug) << "OFI transport (" << fId << "):       OnSend: data=" << msg->GetData() << ",size=" << msg->GetSize();

    // Create and send control message
    auto ctrl = MakeControlMessageWithPmr<PostBuffer>(&fControlMemPool);
    ctrl->size = size;
    auto ctrl_msg = boost::asio::mutable_buffer(ctrl.get(), sizeof(PostBuffer));
    if (fNeedOfiMemoryRegistration) {
        asiofi::memory_region mr(*fOfiDomain, ctrl_msg, asiofi::mr::access::send);
        auto desc = mr.desc();
        fControlEndpoint->send(
            ctrl_msg, desc, [&, ctrl2 = std::move(ctrl)](boost::asio::mutable_buffer) mutable {
                // LOG(debug) << "OFI transport (" << fId << "): >>>>> Control message sent";
            });
    } else {
        fControlEndpoint->send(ctrl_msg,
                               [&, ctrl2 = std::move(ctrl)](boost::asio::mutable_buffer) mutable {
                                   // LOG(debug) << "OFI transport (" << fId << "): >>>>> Control
                                   // message sent";
                               });
    }

    if (size) {
        boost::asio::mutable_buffer buffer(msg->GetData(), size);

        if (fNeedOfiMemoryRegistration) {
            asiofi::memory_region mr(*fOfiDomain, buffer, asiofi::mr::access::send);
            auto desc = mr.desc();

            fDataEndpoint->send(buffer,
                                desc,
                                [&, size, msg2 = std::move(msg), mr2 = std::move(mr)](
                                    boost::asio::mutable_buffer) mutable {
                                    // LOG(debug) << "OFI transport (" << fId << "): >>>>> Data
                                    // buffer sent";
                                    fBytesTx += size;
                                    fMessagesTx++;
                                    fSendSem.async_signal([&](const boost::system::error_code& ec) {
                                        if (!ec) {
                                            // LOG(debug) << "OFI transport (" << fId << "):     >
                                            // Signal fSendSem=" << fSendSem.get_value();
                                        }
                                    });
                                });

        } else {
            fDataEndpoint->send(
                buffer, [&, size, msg2 = std::move(msg)](boost::asio::mutable_buffer) mutable {
                    // LOG(debug) << "OFI transport (" << fId << "): >>>>> Data buffer sent";
                    fBytesTx += size;
                    fMessagesTx++;
                    fSendSem.async_signal([&](const boost::system::error_code& ec) {
                        if (!ec) {
                            // LOG(debug) << "OFI transport (" << fId << "):     > Signal fSendSem="
                            // << fSendSem.get_value();
                        }
                    });
                });
        }
    } else {
        ++fMessagesTx;
        fSendSem.async_signal([&](const boost::system::error_code& ec) {
            if (!ec) {
                // LOG(debug) << "OFI transport (" << fId << "):     > Signal fSendSem=" << fSendSem.get_value();
            }
        });
    }

    boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::SendQueueReader, this));
    // LOG(debug) << "OFI transport (" << fId << "): LEAVE OnSend";
}

auto Socket::RecvControlQueueReader() -> void
{
    fRecvSem.async_wait([&](const boost::system::error_code& ec) {
        if (!ec) {
            auto ctrl = MakeControlMessageWithPmr<PostBuffer>(&fControlMemPool);
            auto ctrl_msg = boost::asio::mutable_buffer(ctrl.get(), sizeof(PostBuffer));

            fControlEndpoint->recv(
                ctrl_msg, [&, ctrl2 = std::move(ctrl)](boost::asio::mutable_buffer) mutable {
                    OnRecvControl(std::move(ctrl2));
                });
        }
    });
}

auto Socket::OnRecvControl(ofi::unique_ptr<PostBuffer> ctrl) -> void
{
    // LOG(debug) << "OFI transport (" << fId << "): ENTER OnRecvControl";

    auto size = ctrl->size;
    // LOG(debug) << "OFI transport (" << fId << "):       OnRecvControl: PostBuffer.size=" << size;

    // Receive data
    if (size) {
        auto msg = fContext.MakeReceiveMessage(size);
        boost::asio::mutable_buffer buffer(msg->GetData(), size);

        if (fNeedOfiMemoryRegistration) {
            asiofi::memory_region mr(*fOfiDomain, buffer, asiofi::mr::access::recv);
            auto desc = mr.desc();

            fDataEndpoint->recv(
                buffer,
                desc,
                [&, msg2 = std::move(msg), mr2 = std::move(mr)](
                    boost::asio::mutable_buffer) mutable {
                    MessagePtr* msgptr(new std::unique_ptr<Message>(std::move(msg2)));
                    fRecvQueueWrite.async_send(
                        azmq::message(boost::asio::const_buffer(msgptr, sizeof(MessagePtr))),
                        [&](const boost::system::error_code& ec, size_t /*bytes_transferred2*/) {
                            if (!ec) {
                                // LOG(debug) << "OFI transport (" << fId
                                // << "): <<<<< Data buffer received, bytes_transferred2="
                                // << bytes_transferred2;
                                fRecvSem.async_signal([&](const boost::system::error_code& ec2) {
                                    if (!ec2) {
                                        // LOG(debug)
                                        // << "OFI transport (" << fId << "):     < Signal
                                        // fRecvSem";
                                    }
                                });
                            }
                        });
                });

        } else {
            fDataEndpoint->recv(
                buffer, [&, msg2 = std::move(msg)](boost::asio::mutable_buffer) mutable {
                    MessagePtr* msgptr(new std::unique_ptr<Message>(std::move(msg2)));
                    fRecvQueueWrite.async_send(
                        azmq::message(boost::asio::const_buffer(msgptr, sizeof(MessagePtr))),
                        [&](const boost::system::error_code& ec, size_t /*bytes_transferred2*/) {
                            if (!ec) {
                                // LOG(debug) << "OFI transport (" << fId
                                // << "): <<<<< Data buffer received, bytes_transferred2="
                                // << bytes_transferred2;
                                fRecvSem.async_signal([&](const boost::system::error_code& ec2) {
                                    if (!ec2) {
                                        // LOG(debug)
                                        // << "OFI transport (" << fId << "):     < Signal
                                        // fRecvSem";
                                    }
                                });
                            }
                        });
                });
        }
    } else {
        fRecvQueueWrite.async_send(
            azmq::message(boost::asio::const_buffer(nullptr, 0)),
            [&](const boost::system::error_code& ec, size_t /*bytes_transferred2*/) {
                if (!ec) {
                    // LOG(debug) << "OFI transport (" << fId
                               // << "): <<<<< Data buffer received, bytes_transferred2="
                               // << bytes_transferred2;
                    fRecvSem.async_signal([&](const boost::system::error_code& ec2) {
                        if (!ec2) {
                            // LOG(debug) << "OFI transport (" << fId << "):     < Signal fRecvSem";
                        }
                    });
                }
            });
    }

    boost::asio::post(fContext.GetIoContext(), std::bind(&Socket::RecvControlQueueReader, this));

    // LOG(debug) << "OFI transport (" << fId << "): LEAVE OnRecvControl";
}

auto Socket::SendImpl(vector<FairMQMessagePtr>& /*msgVec*/, const int /*flags*/, const int /*timeout*/) -> int64_t 
{
    throw SocketError{"Not yet implemented."};
    // const unsigned int vecSize = msgVec.size();
    // int elapsed = 0;
    //
    // // Sending vector typicaly handles more then one part
    // if (vecSize > 1)
    // {
    //     int64_t totalSize = 0;
    //     int nbytes = -1;
    //     bool repeat = false;
    //
    //     while (true && !fInterrupted)
    //     {
    //         for (unsigned int i = 0; i < vecSize; ++i)
    //         {
    //             nbytes = zmq_msg_send(static_cast<FairMQMessageSHM*>(msgVec[i].get())->GetMessage(),
    //                                   fSocket,
    //                                   (i < vecSize - 1) ? ZMQ_SNDMORE|flags : flags);
    //             if (nbytes >= 0)
    //             {
    //                 static_cast<FairMQMessageSHM*>(msgVec[i].get())->fQueued = true;
    //                 size_t size = msgVec[i]->GetSize();
    //
    //                 totalSize += size;
    //             }
    //             else
    //             {
    //                 // according to ZMQ docs, this can only occur for the first part
    //                 if (zmq_errno() == EAGAIN)
    //                 {
    //                     if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
    //                     {
    //                         if (timeout)
    //                         {
    //                             elapsed += fSndTimeout;
    //                             if (elapsed >= timeout)
    //                             {
    //                                 return -2;
    //                             }
    //                         }
    //                         repeat = true;
    //                         break;
    //                     }
    //                     else
    //                     {
    //                         return -2;
    //                     }
    //                 }
    //                 if (zmq_errno() == ETERM)
    //                 {
    //                     LOG(info) << "terminating socket " << fId;
    //                     return -1;
    //                 }
    //                 LOG(error) << "Failed sending on socket " << fId << ", reason: " << zmq_strerror(errno);
    //                 return nbytes;
    //             }
    //         }
    //
    //         if (repeat)
    //         {
    //             continue;
    //         }
    //
    //         // store statistics on how many messages have been sent (handle all parts as a single message)
    //         ++fMessagesTx;
    //         fBytesTx += totalSize;
    //         return totalSize;
    //     }
    //
    //     return -1;
    // } // If there's only one part, send it as a regular message
    // else if (vecSize == 1)
    // {
    //     return Send(msgVec.back(), flags);
    // }
    // else // if the vector is empty, something might be wrong
    // {
    //     LOG(warn) << "Will not send empty vector";
    //     return -1;
    // }
}

auto Socket::ReceiveImpl(vector<FairMQMessagePtr>& /*msgVec*/, const int /*flags*/, const int /*timeout*/) -> int64_t
{
    throw SocketError{"Not yet implemented."};
    // int64_t totalSize = 0;
    // int64_t more = 0;
    // bool repeat = false;
    // int elapsed = 0;
    //
    // while (true)
    // {
    //     // Warn if the vector is filled before Receive() and empty it.
    //     // if (msgVec.size() > 0)
    //     // {
    //     //     LOG(warn) << "Message vector contains elements before Receive(), they will be deleted!";
    //     //     msgVec.clear();
    //     // }
    //
    //     totalSize = 0;
    //     more = 0;
    //     repeat = false;
    //
    //     do
    //     {
    //         FairMQMessagePtr part(new FairMQMessageSHM(fManager, GetTransport()));
    //         zmq_msg_t* msgPtr = static_cast<FairMQMessageSHM*>(part.get())->GetMessage();
    //
    //         int nbytes = zmq_msg_recv(msgPtr, fSocket, flags);
    //         if (nbytes == 0)
    //         {
    //             msgVec.push_back(move(part));
    //         }
    //         else if (nbytes > 0)
    //         {
    //             MetaHeader* hdr = static_cast<MetaHeader*>(zmq_msg_data(msgPtr));
    //             size_t size = 0;
    //             static_cast<FairMQMessageSHM*>(part.get())->fHandle = hdr->fHandle;
    //             static_cast<FairMQMessageSHM*>(part.get())->fSize = hdr->fSize;
    //             static_cast<FairMQMessageSHM*>(part.get())->fRegionId = hdr->fRegionId;
    //             static_cast<FairMQMessageSHM*>(part.get())->fHint = hdr->fHint;
    //             size = part->GetSize();
    //
    //             msgVec.push_back(move(part));
    //
    //             totalSize += size;
    //         }
    //         else if (zmq_errno() == EAGAIN)
    //         {
    //             if (!fInterrupted && ((flags & ZMQ_DONTWAIT) == 0))
    //             {
    //                 if (timeout)
    //                 {
    //                     elapsed += fSndTimeout;
    //                     if (elapsed >= timeout)
    //                     {
    //                         return -2;
    //                     }
    //                 }
    //                 repeat = true;
    //                 break;
    //             }
    //             else
    //             {
    //                 return -2;
    //             }
    //         }
    //         else
    //         {
    //             return nbytes;
    //         }
    //
    //         size_t more_size = sizeof(more);
    //         zmq_getsockopt(fSocket, ZMQ_RCVMORE, &more, &more_size);
    //     }
    //     while (more);
    //
    //     if (repeat)
    //     {
    //         continue;
    //     }
    //
    //     // store statistics on how many messages have been received (handle all parts as a single message)
    //     ++fMessagesRx;
    //     fBytesRx += totalSize;
    //     return totalSize;
    // }
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
