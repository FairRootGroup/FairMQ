/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>

namespace fair
{
namespace mq
{
namespace test
{

class Push : public FairMQDevice
{
  protected:
    auto Run() -> void override
    {
        auto msg = FairMQMessagePtr{NewMessage()};
        Send(msg, "data");
    };
};

} // namespace test
} // namespace mq
} // namespace fair
