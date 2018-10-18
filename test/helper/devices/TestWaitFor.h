/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_WAITFOR_H
#define FAIR_MQ_TEST_WAITFOR_H

#include <FairMQDevice.h>
#include <FairMQLogger.h>

#include <iostream>

namespace fair
{
namespace mq
{
namespace test
{

class TestWaitFor : public FairMQDevice
{
  public:
    void Run()
    {
        std::cout << "hello" << std::endl;
        WaitFor(std::chrono::seconds(60));
    }
};

} // namespace test
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_TEST_WAITFOR_H */
