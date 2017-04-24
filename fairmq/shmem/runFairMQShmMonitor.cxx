/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "FairMQShmMonitor.h"

#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    std::string segmentName = "fairmq_shmem_main";

    if (argc == 2)
    {
        segmentName = argv[1];
    }
    std::cout << "Looking for shared memory segment \"" << segmentName << "\"..." << std::endl;

    fair::mq::shmem::Monitor monitor{segmentName};

    monitor.Run();

    return 0;
}
