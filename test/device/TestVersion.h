/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>
#include <FairMQLogger.h>
#include <options/FairMQProgOptions.h>

namespace fair
{
namespace mq
{
namespace test
{

class TestVersion : public FairMQDevice
{
  public:
    TestVersion(fair::mq::tools::Version version)
        : FairMQDevice(version)
    {}
};

} // namespace test
} // namespace mq
} // namespace fair
