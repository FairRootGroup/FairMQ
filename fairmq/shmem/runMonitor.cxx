/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "Monitor.h"
#include "Common.h"

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
using namespace fair::mq::shmem;

static void daemonize()
{
    // already a daemon?
    if (getppid() == 1) {
        return;
    }

    // Fork
    pid_t pid = fork();
    if (pid < 0) {
        exit(1);
    }

    // If we got a good PID, then we can exit the parent process.
    if (pid > 0) {
        exit(0);
    }

    // Change the file mode mask
    umask(0);

    // Create a new session with the calling process as its leader.
    if (setsid() < 0) {
        exit(1);
    }

    // Change the current working directory. This prevents the current directory from being locked; hence not being able to remove it.
    if ((chdir("/")) < 0) {
        exit(1);
    }

    // Redirect standard files to /dev/null
    if (!freopen("/dev/null", "r", stdin)) {
        cout << "could not redirect stdin to /dev/null" << endl;
    }
    if (!freopen("/dev/null", "w", stdout)) {
        cout << "could not redirect stdout to /dev/null" << endl;
    }
    if (!freopen("/dev/null", "w", stderr)) {
        cout << "could not redirect stderr to /dev/null" << endl;
    }
}

int main(int argc, char** argv)
{
    try {
        string sessionName;
        string shmId;
        bool cleanup = false;
        bool selfDestruct = false;
        bool interactive = false;
        bool viewOnly = false;
        unsigned int timeoutInMS = 5000;
        unsigned int intervalInMS = 100;
        bool runAsDaemon = false;
        bool monitor = false;
        bool debug = false;
        bool cleanOnExit = false;
        bool getShmId = false;
        int userId = -1;

        options_description desc("Options");
        desc.add_options()
            ("session,s"      , value<string>(&sessionName)->default_value("default"),  "Session id")
            ("shmid"          , value<string>(&shmId)->default_value(""),               "Shmem id (if not provided, it is generated out of session id and user id)")
            ("cleanup,c"      , value<bool>(&cleanup)->implicit_value(true),            "Perform cleanup and quit")
            ("self-destruct,x", value<bool>(&selfDestruct)->implicit_value(true),       "Quit after first closing of the memory")
            ("interactive,i"  , value<bool>(&interactive)->implicit_value(true),        "Interactive run")
            ("view,v"         , value<bool>(&viewOnly)->implicit_value(true),           "Run in view only mode")
            ("timeout,t"      , value<unsigned int>(&timeoutInMS)->default_value(5000), "Heartbeat timeout in milliseconds")
            ("daemonize,d"    , value<bool>(&runAsDaemon)->implicit_value(true),        "Daemonize the monitor")
            ("monitor,m"      , value<bool>(&monitor)->implicit_value(true),            "Run in monitoring mode")
            ("debug,b"        , value<bool>(&debug)->implicit_value(true),              "Debug - Print a list of messages)")
            ("clean-on-exit,e", value<bool>(&cleanOnExit)->implicit_value(true),        "Perform cleanup on exit")
            ("interval"       , value<unsigned int>(&intervalInMS)->default_value(100), "Output interval for interactive/view-only mode")
            ("get-shmid"      , value<bool>(&getShmId)->implicit_value(true),           "Translate given session id and user id to a shmem id (uses current user id if none provided)")
            ("user-id"        , value<int>(&userId)->default_value(-1),                 "User id (used with --get-shmid)")
            ("help,h", "Print help");

        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            cout << "FairMQ Shared Memory Monitor" << endl << desc << endl;
            return 0;
        }

        notify(vm);

        if (runAsDaemon) {
            daemonize();
        }

        if (getShmId) {
            if (userId == -1) {
                cout << "shmem id for session '" << sessionName << "' and current user id " << geteuid()
                     << " is: " << makeShmIdStr(sessionName) << endl;
            } else {
                cout << "shmem id for session '" << sessionName << "' and user id " << userId
                     << " is: " << makeShmIdStr(sessionName, to_string(userId)) << endl;
            }
            return 0;
        }

        if (shmId.empty()) {
            shmId = makeShmIdStr(sessionName);
        }

        if (cleanup) {
            Monitor::CleanupFull(ShmId{shmId});
            return 0;
        }

        if (debug) {
            Monitor::PrintDebugInfo(ShmId{shmId});
            return 0;
        }

        cout << "Starting shared memory monitor for session: \"" << sessionName << "\" (shmId: " << shmId << ")..." << endl;

        if (!viewOnly && !interactive && !monitor) {
            // if neither of the run modes are selected, use view only mode.
            viewOnly = true;
        }

        Monitor shmmonitor(shmId, selfDestruct, interactive, viewOnly, timeoutInMS, intervalInMS, monitor, cleanOnExit);

        if (interactive || !viewOnly) {
            shmmonitor.CatchSignals();
        }
        shmmonitor.Run();
    } catch (Monitor::DaemonPresent& dp) {
        return 0;
    } catch (exception& e) {
        cerr << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit" << endl;
        return 2;
    }

    return 0;
}
