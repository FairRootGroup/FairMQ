/********************************************************************************
 *    Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_ZMQ_ZMSG_H
#define FAIR_MQ_ZMQ_ZMSG_H

#include <cstddef>          // for std::size_t
#include <fairmq/Error.h>   // for assertm
#include <new>              // for std::bad_alloc
#include <zmq.h>            // for zmq_*

namespace fair::mq::zmq {

// Wraps a `zmq_msg_t` C object as a C++ type:
// * `zmq_msg_init` -> C++ default ctor
// * `zmq_msg_init_size` -> C++ ctor
// * `zmq_msg_init_data` -> C++ ctor
// * `zmq_msg_init_size` + `memcpy` -> C++ copy ctor
// * `zmq_msg_close` + `zmq_msg_init_size` + `memcpy` -> C++ copy assignment
// * `zmq_msg_init` + `zmq_msg_move`` -> C++ move ctor
// * `zmq_msg_move` -> C++ move assignment
// * `zmq_msg_close` -> C++ dtor
// * access the underlying `zmq_msg_t` via `Msg() [const] -> zmq_msg_t*`
//   the const overload does a `const_cast<zmq_msg_t*>`, because the
//   C interfaces do not model constness
// * `zmq_msg_data` -> `Data() -> void*`
// * `zmq_msg_size` -> `Size() -> std::size_t`
struct ZMsg
{
    ZMsg() noexcept
    {
        [[maybe_unused]] auto const rc = zmq_msg_init(Msg());
        assertm(rc == 0, "msg init successful");   // NOLINT
    }
    explicit ZMsg(std::size_t size)
    {
        auto const rc = zmq_msg_init_size(Msg(), size);
        if (rc == -1) {
            throw std::bad_alloc{};
        }
    }
    explicit ZMsg(void* data,
                  std::size_t size,
                  zmq_free_fn* freefn = nullptr,
                  void* hint = nullptr)
    {
        auto const rc = zmq_msg_init_data(Msg(), data, size, freefn, hint);
        if (rc == -1) {
            throw std::bad_alloc{};
        }
    }
    ~ZMsg() noexcept
    {
        [[maybe_unused]] auto const rc = zmq_msg_close(Msg());
        assertm(rc == 0, "msg close successful");   // NOLINT
    }
    ZMsg(const ZMsg& other) = delete;
    ZMsg(ZMsg&& other) noexcept
    {
        [[maybe_unused]] auto rc = zmq_msg_init(Msg());
        assertm(rc == 0, "msg init successful");   // NOLINT
        rc = zmq_msg_move(Msg(), other.Msg());
        assertm(rc == 0, "msg move successful");   // NOLINT
    }
    ZMsg& operator=(const ZMsg& rhs) = delete;
    ZMsg& operator=(ZMsg&& rhs) noexcept
    {
        [[maybe_unused]] auto const rc = zmq_msg_move(Msg(), rhs.Msg());
        assertm(rc == 0, "msg move successful");   // NOLINT
        return *this;
    }

    zmq_msg_t* Msg() noexcept { return &fMsg; }
    zmq_msg_t* Msg() const noexcept
    {
        return const_cast<zmq_msg_t*>(&fMsg);   // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }
    void* Data() const noexcept { return zmq_msg_data(Msg()); }
    std::size_t Size() const noexcept { return zmq_msg_size(Msg()); }

  private:
    zmq_msg_t fMsg{};
};

}   // namespace fair::mq::zmq

#endif /* FAIR_MQ_ZMQ_ZMSG_H */
