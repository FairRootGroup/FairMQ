/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_RUNNER_H
#define FAIR_MQ_TEST_RUNNER_H

#include <string>

namespace fair::mq::test
{

extern std::string runTestDevice; /// Path to test device executable.
extern std::string mqConfig; /// Path to FairMQ device config file.

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_RUNNER_H */
