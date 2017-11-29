/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "Monitor.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <string>

using namespace std;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        string sessionName;
        bool cleanup = false;
        bool selfDestruct = false;
        bool interactive = false;
        unsigned int timeoutInMS;

        options_description desc("Options");
        desc.add_options()
            ("session", value<string>(&sessionName)->default_value("default"), "Name of the session which to monitor")
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

        sessionName.resize(8, '_'); // shorten the session name, to accommodate for name size limit on some systems (MacOS)

        if (cleanup)
        {
            cout << "Cleaning up \"" << sessionName << "\"..." << endl;
            fair::mq::shmem::Monitor::Cleanup(sessionName);
            fair::mq::shmem::Monitor::RemoveQueue("fmq_shm_" + sessionName + "_control_queue");
            return 0;
        }

        cout << "Starting shared memory monitor for session: \"" << sessionName << "\"..." << endl;

        fair::mq::shmem::Monitor monitor{sessionName, selfDestruct, interactive, timeoutInMS};

        monitor.CatchSignals();
        monitor.Run();
    }
    catch (exception& e)
    {
        cerr << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit" << endl;
        return 2;
    }

    return 0;
}
