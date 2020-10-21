/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_MSG_H_
#define FAIR_MQ_MSG_H_

#include "FairMQMessage.h"
#include <fairmq/Buffer.h>

#include <type_traits>
#include <vector>

namespace fair
{
namespace mq
{

class Msg
{
  private:
    using container = std::vector<Buffer>;

  public:
    Msg() : fBuffers() {};
    Msg(size_t elements) : fBuffers() { fBuffers.reserve(elements); };

    Msg(const Msg&) = delete;
    Msg(Msg&) = delete;
    Msg& operator=(const Msg&) = delete;
    Msg& operator=(Msg&) = delete;

    Msg(Msg&& p) = default;
    Msg& operator=(Msg&&) = default;

    template<typename... Ts, typename = std::enable_if_t<
                (std::is_same_v<std::decay_t<Ts>, Msg> && ...) || // with c++20 use std::is_same_v<std::remove_cvref_t<Ts>, Msg>
                (std::is_convertible_v<Ts, Buffer> && ...)>>
    Msg(Ts&&... buffers) : fBuffers()
    {
        fBuffers.reserve(sizeof...(Ts));
        Add(std::forward<Ts>(buffers)...);
    }

    ~Msg() {};

    Buffer& Add(Buffer buffer) { return fBuffers.emplace_back(std::move(buffer)); }

    template<typename... Ts>
    void Add(Buffer first, Ts... remaining)
    {
        fBuffers.emplace_back(std::move(first));
        if constexpr (sizeof...(Ts) > 0) Add(std::forward<Ts>(remaining)...);
    }

    void Add(Msg other)
    {
        for (auto& buffer : other) {
            fBuffers.emplace_back(std::move(buffer));
        }
    }

    Buffer& operator[](int i) { return fBuffers[i]; }
    Buffer& At(int i) { return fBuffers.at(i); }

    size_t Size() const { return fBuffers.size(); }
    void Reserve(size_t cap) { fBuffers.reserve(cap); }

    container fBuffers;

    // forward container iterators
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;

    auto begin() -> decltype(fBuffers.begin()) { return fBuffers.begin(); }
    auto cbegin() -> decltype(fBuffers.cbegin()) { return fBuffers.cbegin(); }
    auto end() -> decltype(fBuffers.end()) { return fBuffers.end(); }
    auto cend() -> decltype(fBuffers.cend()) { return fBuffers.cend(); }
};

} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_MSG_H_ */
