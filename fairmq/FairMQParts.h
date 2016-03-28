/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQPARTS_H_
#define FAIRMQPARTS_H_

#include <vector>
#include <memory> // unique_ptr

#include "FairMQTransportFactory.h"
#include "FairMQMessage.h"
#include "FairMQLogger.h"
#include "zeromq/FairMQMessageZMQ.h"
//class FairMQMessageZMQ;
class FairMQMessageNN;
namespace fairmq
{
    namespace transport
    {
        struct ZMQ{};
        struct NN{};
    }
}

/// FairMQParts is a lightweight convenience wrapper around a vector of unique pointers to FairMQMessage, used for sending multi-part messages

class FairMQParts
{
  public:
    /// Default constructor
    FairMQParts() : fParts() {};
    /// Copy Constructor
    FairMQParts(const FairMQParts&) = delete;
    /// Move constructor
    FairMQParts(FairMQParts&& p) = default;
    /// Assignment operator
    FairMQParts& operator=(const FairMQParts&) = delete;
    /// Default destructor
    ~FairMQParts() {};

    /// Adds part (FairMQMessage) to the container
    /// @param msg message pointer (for example created with NewMessage() method of FairMQDevice)
    inline void AddPart(FairMQMessage* msg)
    {
        fParts.push_back(std::unique_ptr<FairMQMessage>(msg));
    }
    

    inline void AddPart(std::unique_ptr<FairMQMessage>& msg)
    {
        fParts.push_back(std::move(msg));
    }

    template<typename Serializer, typename DataType>
    inline void AddPart(DataType&& data)
    {
        std::unique_ptr<FairMQMessage> msg(new FairMQMessageZMQ());
        Serializer().Serialize(msg, std::forward<DataType>(data));
        fParts.push_back(std::move(msg));
    }



    /// Adds part (std::unique_ptr<FairMQMessage>) to the container (move)
    /// @param msg unique pointer to FairMQMessage
    inline void AddPart(std::unique_ptr<FairMQMessage> msg)
    {
        fParts.push_back(std::move(msg));
    }

    /// Get reference to part in the container at index (without bounds check)
    /// @param index container index
    inline FairMQMessage& operator[](const int index) { return *(fParts[index]); }

    /// Get reference to unique pointer to part in the container at index (with bounds check)
    /// @param index container index
    inline std::unique_ptr<FairMQMessage>& At(const int index) { return fParts.at(index); }


    inline FairMQMessage& At_ref(const int index) { return *(fParts.at(index)); }

    template<typename Deserializer, typename DataType>
    inline void At(DataType&& data, const int index)
    {
        Deserializer().Deserialize(fParts.at(index), std::forward<DataType>(data));
    }

    inline std::unique_ptr<FairMQMessage>& At_ptr(const int index) { return fParts.at(index); }
    /// Get number of parts in the container
    /// @return number of parts in the container
    inline int Size() const { return fParts.size(); }

    std::vector<std::unique_ptr<FairMQMessage>> fParts;
};

#endif /* FAIRMQPARTS_H_ */
