/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <TestEnvironment.h>

#include <gtest/gtest.h>
#include <stdlib.h>

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    setenv("FAIRMQ_PATH", FAIRMQ_TEST_ENVIRONMENT, 0);
    return RUN_ALL_TESTS();
}
