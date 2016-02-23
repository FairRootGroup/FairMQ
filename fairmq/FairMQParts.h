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

class FairMQParts
{
  public:
    /// Default constructor
    FairMQParts() {};
    /// Copy Constructor
    FairMQParts(const FairMQParts&) = delete;
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

    /// Get reference to part in the container at index (without bounds check)
    /// @param index container index
    inline FairMQMessage& operator[](const int index) { return *(fParts[index]); }

    /// Get reference to part in the container at index (with bounds check)
    /// @param index container index
    inline FairMQMessage& At(const int index) { return *(fParts.at(index)); }

    /// Get number of parts in the container
    /// @return number of parts in the container
    inline int Size() const { return fParts.size(); }

    std::vector<std::unique_ptr<FairMQMessage>> fParts;
};

#endif /* FAIRMQPARTS_H_ */
