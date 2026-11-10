/********************************************************************************
 * Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH       *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/Version.h>

#include <iostream>

int main()
{
    fair::mq::Device device;
    std::cout << "FairMQ " << FAIRMQ_VERSION << " consumer ok\n";
    return 0;
}
