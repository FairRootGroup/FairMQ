/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQPARTS_H_
#define FAIRMQPARTS_H_

#include "FairMQTransportFactory.h"
#include "FairMQMessage.h"

#include <vector>
#include <memory> // unique_ptr

/// FairMQParts is a lightweight convenience wrapper around a vector of unique pointers to FairMQMessage, used for sending multi-part messages

class FairMQParts
{
  private:
    using container = std::vector<std::unique_ptr<FairMQMessage>>;

  public:
    /// Default constructor
    FairMQParts() : fParts() {};
    /// Copy Constructor
    FairMQParts(const FairMQParts&) = delete;
    /// Move constructor
    FairMQParts(FairMQParts&& p) = default;
    /// Assignment operator
    FairMQParts& operator=(const FairMQParts&) = delete;
    /// Constructor from argument pack of std::unique_ptr<FairMQMessage> rvalues
    template <typename... Ts>
    FairMQParts(Ts&&... messages) : fParts() { AddPart(std::forward<Ts>(messages)...); }
    /// Default destructor
    ~FairMQParts() {};

    /// Adds part (FairMQMessage) to the container
    /// @param msg message pointer (for example created with NewMessage() method of FairMQDevice)
    void AddPart(FairMQMessage* msg)
    {
        fParts.push_back(std::unique_ptr<FairMQMessage>(msg));
    }

    /// Adds part (std::unique_ptr<FairMQMessage>&) to the container (move)
    /// @param msg unique pointer to FairMQMessage
    /// rvalue ref (move required when passing argument)
    void AddPart(std::unique_ptr<FairMQMessage>&& msg)
    {
        fParts.push_back(std::move(msg));
    }

    /// Add variable list of parts to the container (move)
    template <typename... Ts>
    void AddPart(std::unique_ptr<FairMQMessage>&& first, Ts&&... remaining)
    {
        AddPart(std::move(first));
        AddPart(std::forward<Ts>(remaining)...);
    }

    /// Add content of another object by move
    void AddPart(FairMQParts&& other)
    {
        container parts = std::move(other.fParts);
        for (auto& part : parts) {
            fParts.push_back(std::move(part));
        }
    }

    /// Get reference to part in the container at index (without bounds check)
    /// @param index container index
    FairMQMessage& operator[](const int index) { return *(fParts[index]); }

    /// Get reference to unique pointer to part in the container at index (with bounds check)
    /// @param index container index
    std::unique_ptr<FairMQMessage>& At(const int index) { return fParts.at(index); }

    // ref version
    FairMQMessage& AtRef(const int index) { return *(fParts.at(index)); }

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

#endif /* FAIRMQPARTS_H_ */
