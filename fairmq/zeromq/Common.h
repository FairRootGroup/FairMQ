/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_ZMQ_COMMON_H
#define FAIR_MQ_ZMQ_COMMON_H

#include <fairlogger/Logger.h>
#include <fairmq/Error.h>
#include <fairmq/tools/Strings.h>
#include <stdexcept>
#include <string_view>
#include <zmq.h>

namespace fair::mq::zmq
{

struct Error : std::runtime_error { using std::runtime_error::runtime_error; };

inline bool Bind(void* socket, const std::string& address, const std::string& id)
{
    // LOG(debug) << "Binding socket " << id << " on " << address;
    if (zmq_bind(socket, address.c_str()) != 0) {
        if (errno == EADDRINUSE) {
            // do not print error in this case, this is handled upstream in case no
            // connection could be established after trying a number of random ports from a range.
            size_t protocolPos = address.find(':');
            std::string protocol = address.substr(0, protocolPos);
            if (protocol == "tcp") {
              return false;
            }
        } else if (errno == EACCES) {
            // check if TCP port 1 was given, if yes then it will be handeled upstream, print debug only
            size_t protocolPos = address.find(':');
            std::string protocol = address.substr(0, protocolPos);
            if (protocol == "tcp") {
                size_t portPos = address.rfind(':');
                std::string port = address.substr(portPos + 1);
                if (port == "1") {
                    LOG(debug) << "Failed binding socket " << id << ", address: " << address << ", reason: " << zmq_strerror(errno);
                    return false;
                }
            }
        }
        LOG(error) << "Failed binding socket " << id << ", address: " << address << ", reason: " << zmq_strerror(errno);
        return false;
    }
    return true;
}

inline bool Connect(void* socket, const std::string& address, const std::string& id)
{
    // LOG(debug) << "Connecting socket " << id << " on " << address;
    if (zmq_connect(socket, address.c_str()) != 0) {
        LOG(error) << "Failed connecting socket " << id << ", address: " << address << ", reason: " << zmq_strerror(errno);
        return false;
    }
    return true;
}

inline bool ShouldRetry(int flags, int socketTimeout, int userTimeout, int& elapsed)
{
    if ((flags & ZMQ_DONTWAIT) == 0) {
        if (userTimeout > 0) {
            elapsed += socketTimeout;
            if (elapsed >= userTimeout) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

inline int HandleErrors(const std::string& id)
{
    if (zmq_errno() == ETERM) {
        LOG(debug) << "Terminating socket " << id;
        return static_cast<int>(TransferCode::error);
    } else {
        LOG(error) << "Failed transfer on socket " << id << ", errno: " << errno << ", reason: " << zmq_strerror(errno);
        return static_cast<int>(TransferCode::error);
    }
}

/// Lookup table for various zmq constants
inline auto getConstant(std::string_view constant) -> int
{
    if (constant.empty()) { return 0; }
    if (constant == "sub") { return ZMQ_SUB; }
    if (constant == "pub") { return ZMQ_PUB; }
    if (constant == "xsub") { return ZMQ_XSUB; }
    if (constant == "xpub") { return ZMQ_XPUB; }
    if (constant == "push") { return ZMQ_PUSH; }
    if (constant == "pull") { return ZMQ_PULL; }
    if (constant == "req") { return ZMQ_REQ; }
    if (constant == "rep") { return ZMQ_REP; }
    if (constant == "dealer") { return ZMQ_DEALER; }
    if (constant == "router") { return ZMQ_ROUTER; }
    if (constant == "pair") { return ZMQ_PAIR; }

    if (constant == "snd-hwm") { return ZMQ_SNDHWM; }
    if (constant == "rcv-hwm") { return ZMQ_RCVHWM; }
    if (constant == "snd-size") { return ZMQ_SNDBUF; }
    if (constant == "rcv-size") { return ZMQ_RCVBUF; }
    if (constant == "snd-more") { return ZMQ_SNDMORE; }
    if (constant == "rcv-more") { return ZMQ_RCVMORE; }

    if (constant == "linger") { return ZMQ_LINGER; }
    if (constant == "no-block") { return ZMQ_DONTWAIT; }
    if (constant == "snd-more no-block") { return ZMQ_DONTWAIT|ZMQ_SNDMORE; }

    if (constant == "fd") { return ZMQ_FD; }
    if (constant == "events") { return ZMQ_EVENTS; }
    if (constant == "pollin") { return ZMQ_POLLIN; }
    if (constant == "pollout") { return ZMQ_POLLOUT; }

    throw Error(tools::ToString("getConstant called with an invalid argument: ", constant));
}

/// Create a zmq event monitor socket pair, and configure/connect the reading socket
/// @return reading monitor socket
inline auto makeMonitorSocket(void* zmqCtx, void* socketToMonitor, std::string_view id) -> void*
{
    assertm(zmqCtx, "Given zmq context exists");   // NOLINT

    if (!socketToMonitor) {   // nothing to do in this case
        return nullptr;
    }

    auto const address(tools::ToString("inproc://", id));
    {   // Create writing monitor socket on socket to be monitored and subscribe
        // to all relevant events needed to compute connected peers
        // from http://api.zeromq.org/master:zmq-socket-monitor:
        //
        // ZMQ_EVENT_CONNECTED - The socket has successfully connected to a remote peer. The event
        //   value is the file descriptor (FD) of the underlying network socket. Warning: there is
        //   no guarantee that the FD is still valid by the time your code receives this event.
        // ZMQ_EVENT_ACCEPTED - The socket has accepted a connection from a remote peer. The event
        //   value is the FD of the underlying network socket. Warning: there is no guarantee that
        //   the FD is still valid by the time your code receives this event.
        // ZMQ_EVENT_DISCONNECTED - The socket was disconnected unexpectedly. The event value is the
        //   FD of the underlying network socket. Warning: this socket will be closed.
        [[maybe_unused]] auto const rc =
            zmq_socket_monitor(socketToMonitor,
                               address.c_str(),
                               ZMQ_EVENT_CONNECTED | ZMQ_EVENT_ACCEPTED | ZMQ_EVENT_DISCONNECTED);
        assertm(rc == 0, "Creating writing monitor socket succeeded");   // NOLINT
    }
    // Create reading monitor socket
    auto mon(zmq_socket(zmqCtx, ZMQ_PAIR));
    assertm(mon, "Creating reading monitor socker succeeded");   // NOLINT
    {   // Set receive queue size to unlimited on reading socket
        // Rationale: In the current implementation this is needed for correctness, because
        //            we do not have any thread that emptys the receive queue regularly.
        //            Progress only happens, when a user calls GetNumberOfConnectedPeers()`.
        //            The assumption here is, that not too many events will pile up anyways.
        int const unlimited(0);
        [[maybe_unused]] auto const rc = zmq_setsockopt(mon, ZMQ_RCVHWM, &unlimited, sizeof(unlimited));
        assertm(rc == 0, "Setting rcv queue size to unlimited succeeded");   // NOLINT
    }
    {   // Connect the reading monitor socket
        [[maybe_unused]] auto const rc = zmq_connect(mon, address.c_str());
        assertm(rc == 0, "Connecting reading monitor socket succeeded");   // NOLINT
    }
    return mon;
}

/// Read pending zmq monitor event in a non-blocking fashion.
/// @return event id or -1 for no event pending
inline auto getMonitorEvent(void* monitorSocket) -> int
{
    assertm(monitorSocket, "zmq monitor socket exists");   // NOLINT

    // First frame in message contains event id
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    {
        auto const size = zmq_msg_recv(&msg, monitorSocket, ZMQ_DONTWAIT);
        if (size == -1) {
            return -1;   // no event pending
        }
        assertm(size >= 2, "At least two bytes were received");   // NOLINT
    }

    // Unpack event id
    auto const event = *static_cast<uint16_t*>(zmq_msg_data(&msg));

    // No unpacking of the event value needed for now

    // Second frame in message contains event address
    assertm(zmq_msg_more(&msg), "A second frame is pending");   // NOLINT
    zmq_msg_close(&msg);
    zmq_msg_t msg2;
    zmq_msg_init(&msg2);
    {
        [[maybe_unused]] auto const rc = zmq_msg_recv(&msg2, monitorSocket, 0);
        assertm(rc >= 0, "second monitor event frame successfully received");   // NOLINT
    }
    assertm(!zmq_msg_more(&msg2), "No more frames are pending");   // NOLINT
    // No unpacking of the event address needed for now
    zmq_msg_close(&msg2);

    return event;
}

/// Compute updated connected peers count by consuming pending events from a zmq monitor socket
/// @return updated connected peers count
inline auto updateNumberOfConnectedPeers(unsigned long count, void* monitorSocket) -> unsigned long
{
    if (monitorSocket == nullptr) {
        return count;
    }

    int event = getMonitorEvent(monitorSocket);
    while (event >= 0) {
        switch (event) {
            case ZMQ_EVENT_CONNECTED:
            case ZMQ_EVENT_ACCEPTED:
                ++count;
                break;
            case ZMQ_EVENT_DISCONNECTED:
                if (count > 0) {
                    --count;
                } else {
                    LOG(warn) << "Computing connected peers would result in negative count! Some event was missed!";
                }
                break;
            default:
                break;
        }
        event = getMonitorEvent(monitorSocket);
    }
    return count;
}

} // namespace fair::mq::zmq

#endif /* FAIR_MQ_ZMQ_COMMON_H */
