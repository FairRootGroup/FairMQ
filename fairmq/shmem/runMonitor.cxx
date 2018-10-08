/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include <fairmq/shmem/Monitor.h>

#include <boost/program_options.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <string>

using namespace std;
using namespace boost::program_options;

static void daemonize()
{
    // already a daemon?
    // if (getppid() == 1) return;

    // Fork off the parent process
    // pid_t pid = fork();
    // if (pid < 0) exit(1);

    // If we got a good PID, then we can exit the parent process.
    // if (pid > 0) exit(0);

    // Change the file mode mask
    umask(0);

    // Create a new SID for the child process
    if (setsid() < 0)
    {
        exit(1);
    }

    // Change the current working directory. This prevents the current directory from being locked; hence not being able to remove it.
    if ((chdir("/")) < 0)
    {
        exit(1);
    }

    // Redirect standard files to /dev/null
    if (!freopen("/dev/null", "r", stdin))
    {
        cout << "could not redirect stdin to /dev/null" << endl;
    }
    if (!freopen("/dev/null", "w", stdout))
    {
        cout << "could not redirect stdout to /dev/null" << endl;
    }
    if (!freopen("/dev/null", "w", stderr))
    {
        cout << "could not redirect stderr to /dev/null" << endl;
    }
}

int main(int argc, char** argv)
{
    try
    {
        string sessionName;
        bool cleanup = false;
        bool selfDestruct = false;
        bool interactive = false;
        unsigned int timeoutInMS = 5000;
        bool runAsDaemon = false;
        bool cleanOnExit = false;

        options_description desc("Options");
        desc.add_options()
            ("session,s", value<string>(&sessionName)->default_value("default"), "Name of the session which to monitor")
            ("cleanup,c", value<bool>(&cleanup)->implicit_value(true), "Perform cleanup and quit")
            ("self-destruct,x", value<bool>(&selfDestruct)->implicit_value(true), "Quit after first closing of the memory")
            ("interactive,i", value<bool>(&interactive)->implicit_value(true), "Interactive run")
            ("timeout,t", value<unsigned int>(&timeoutInMS)->default_value(5000), "Heartbeat timeout in milliseconds")
            ("daemonize,d", value<bool>(&runAsDaemon)->implicit_value(true), "Daemonize the monitor")
            ("clean-on-exit,e", value<bool>(&cleanOnExit)->implicit_value(true), "Perform cleanup on exit")
            ("help,h", "Print help");

        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            cout << "FairMQ Shared Memory Monitor" << endl << desc << endl;
            return 0;
        }

        notify(vm);

        sessionName.resize(8, '_'); // shorten the session name, to accommodate for name size limit on some systems (MacOS)

        if (runAsDaemon)
        {
            daemonize();
        }

        if (cleanup)
        {
            cout << "Cleaning up \"" << sessionName << "\"..." << endl;
            fair::mq::shmem::Monitor::Cleanup(sessionName);
            fair::mq::shmem::Monitor::RemoveQueue("fmq_" + sessionName + "_cq");
            return 0;
        }

        cout << "Starting shared memory monitor for session: \"" << sessionName << "\"..." << endl;

        fair::mq::shmem::Monitor monitor{sessionName, selfDestruct, interactive, timeoutInMS, runAsDaemon, cleanOnExit};

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
