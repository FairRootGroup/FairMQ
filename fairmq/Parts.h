/********************************************************************************
 * Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PARTS_H
#define FAIR_MQ_PARTS_H

#include <algorithm>          // std::move
#include <fairmq/Message.h>   // fair::mq::MessagePtr
#include <iterator>           // std::back_inserter
#include <utility>            // std::move, std::forward
#include <vector>             // std::vector

namespace fair::mq {

/// fair::mq::Parts is a lightweight move-only convenience wrapper around a vector of unique pointers to
/// Message, used for sending multi-part messages
struct Parts
{
    using container = std::vector<MessagePtr>;
    using size_type = container::size_type;
    using reference = container::reference;
    using const_reference = container::const_reference;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;

    Parts() noexcept(noexcept(container())) = default;
    Parts(const Parts&) = delete;
    Parts& operator=(const Parts&) = delete;
    Parts(Parts&&) = default;
    Parts& operator=(Parts&&) = default;
    ~Parts() = default;

    template<typename... Ps>
    Parts(Ps&&... parts)
    {
        fParts.reserve(sizeof...(Ps));
        AddPart(std::forward<Ps>(parts)...);
    }

    [[deprecated("Avoid owning raw pointer args, use AddPart(MessagePtr) instead.")]]
    void AddPart(Message* msg) { fParts.push_back(MessagePtr(msg)); }
    void AddPart(MessagePtr msg) { fParts.push_back(std::move(msg)); }

    template<typename... Ts>
    void AddPart(MessagePtr first, Ts&&... remaining)
    {
        AddPart(std::move(first));
        AddPart(std::forward<Ts>(remaining)...);
    }

    void AddPart(Parts parts)
    {
        if (fParts.empty()) {
            fParts = std::move(parts.fParts);
        } else {
            fParts.reserve(parts.Size() + fParts.size());
            std::move(std::begin(parts), std::end(parts), std::back_inserter(fParts));
        }
    }

    Message& operator[](size_type index) { return *(fParts[index]); }
    Message const& operator[](size_type index) const { return *(fParts[index]); }
    // TODO: For consistency with the STL interfaces, operator[] should not dereference,
    //       but I have no good idea how to fix this.
    // reference operator[](size_type index) { return fParts[index]; }
    // const_reference operator[](size_type index) const { return fParts[index]; }

    [[deprecated("Redundant, dereference at call site e.g. '*(parts.At(index))' instead.")]]
    Message& AtRef(size_type index) { return *(fParts.at(index)); }
    reference At(size_type index) { return fParts.at(index); }
    const_reference At(size_type index) const { return fParts.at(index); }

    size_type Size() const noexcept { return fParts.size(); }
    bool Empty() const noexcept { return fParts.empty(); }
    void Clear() noexcept { fParts.clear(); }

    // range access
    iterator begin() noexcept { return fParts.begin(); }
    const_iterator begin() const noexcept { return fParts.begin(); }
    const_iterator cbegin() const noexcept { return fParts.cbegin(); }
    iterator end() noexcept { return fParts.end(); }
    const_iterator end() const noexcept { return fParts.end(); }
    const_iterator cend() const noexcept { return fParts.cend(); }

    container fParts{};
};

}   // namespace fair::mq

using FairMQParts [[deprecated("Use fair::mq::Parts")]] = fair::mq::Parts;

#endif /* FAIR_MQ_PARTS_H */
