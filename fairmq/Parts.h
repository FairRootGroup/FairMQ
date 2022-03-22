/********************************************************************************
 * Copyright (C) 2014-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PARTS_H
#define FAIR_MQ_PARTS_H

#include <fairmq/Message.h>
#include <memory>   // unique_ptr
#include <vector>

namespace fair::mq {

/// fair::mq::Parts is a lightweight convenience wrapper around a vector of unique pointers to
/// Message, used for sending multi-part messages
class Parts
{
  private:
    using container = std::vector<MessagePtr>;

  public:
    Parts() = default;
    Parts(const Parts&) = delete;
    Parts(Parts&&) = default;
    template<typename... Ts>
    Parts(Ts&&... messages) { AddPart(std::forward<Ts>(messages)...); }
    Parts& operator=(const Parts&) = delete;
    Parts& operator=(Parts&&) = default;
    ~Parts() = default;

    /// Adds part (Message) to the container
    /// @param msg message pointer (for example created with NewMessage() method of Device)
    void AddPart(Message* msg) { fParts.push_back(MessagePtr(msg)); }

    /// Adds part to the container (move)
    /// @param msg unique pointer to Message
    /// rvalue ref (move required when passing argument)
    void AddPart(MessagePtr&& msg) { fParts.push_back(std::move(msg)); }

    /// Add variable list of parts to the container (move)
    template<typename... Ts>
    void AddPart(MessagePtr&& first, Ts&&... remaining)
    {
        AddPart(std::move(first));
        AddPart(std::forward<Ts>(remaining)...);
    }

    /// Add content of another object by move
    void AddPart(Parts&& other)
    {
        container parts = std::move(other.fParts);
        for (auto& part : parts) {
            fParts.push_back(std::move(part));
        }
    }

    /// Get reference to part in the container at index (without bounds check)
    /// @param index container index
    Message& operator[](const int index) { return *(fParts[index]); }

    /// Get reference to unique pointer to part in the container at index (with bounds check)
    /// @param index container index
    MessagePtr& At(const int index) { return fParts.at(index); }

    // ref version
    Message& AtRef(const int index) { return *(fParts.at(index)); }

    /// Get number of parts in the container
    /// @return number of parts in the container
    int Size() const { return fParts.size(); }

    container fParts;

    // forward container iterators
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    auto begin() -> decltype(fParts.begin()) { return fParts.begin(); }
    auto end() -> decltype(fParts.end()) { return fParts.end(); }
    auto cbegin() -> decltype(fParts.cbegin()) { return fParts.cbegin(); }
    auto cend() -> decltype(fParts.cend()) { return fParts.cend(); }
};

}   // namespace fair::mq

using FairMQParts [[deprecated("Use fair::mq::Parts")]] = fair::mq::Parts;

#endif /* FAIR_MQ_PARTS_H */
