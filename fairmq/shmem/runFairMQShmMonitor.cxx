/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "FairMQShmMonitor.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <string>

using namespace std;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        string segmentName;
        bool cleanup = false;
        bool selfDestruct = false;
        bool interactive = false;
        unsigned int timeoutInMS;

        options_description desc("Options");
        desc.add_options()
            ("segment-name", value<string>(&segmentName)->default_value("fairmq_shmem_main"), "Name of the shared memory segment")
            ("cleanup", value<bool>(&cleanup)->implicit_value(true), "Perform cleanup and quit")
            ("self-destruct", value<bool>(&selfDestruct)->implicit_value(true), "Quit after first closing of the memory")
            ("interactive", value<bool>(&interactive)->implicit_value(true), "Interactive run")
            ("timeout", value<unsigned int>(&timeoutInMS)->default_value(5000), "Heartbeat timeout in milliseconds")
            ("help", "Print help");

        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            cout << "FairMQ Shared Memory Monitor" << endl << desc << endl;
            return 0;
        }

        notify(vm);

        if (cleanup)
        {
            cout << "Cleaning up \"" << segmentName << "\"..." << endl;
            fair::mq::shmem::Monitor::Cleanup(segmentName);
            return 0;
        }

        cout << "Starting monitor for shared memory segment: \"" << segmentName << "\"..." << endl;

        fair::mq::shmem::Monitor monitor{segmentName, selfDestruct, interactive, timeoutInMS};

        monitor.Run();
    }
    catch (exception& e)
    {
        cerr << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit" << endl;
        return 2;
    }

    return 0;
}
