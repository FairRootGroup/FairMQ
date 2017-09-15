/********************************************************************************
 *  Copyright (C) 2014-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <dds_intercom.h>

#include <termios.h> // raw mode console input

#include <iostream>
#include <exception>
#include <sstream>
#include <thread>
#include <atomic>
#include <unistd.h>

using namespace std;
using namespace dds::intercom_api;

void PrintControlsHelp()
{
    cout << "Use keys to control the devices:" << endl;
    cout << "[c] check states, [h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device" << endl;
    cout << "To quit press Ctrl+C" << endl;
}

int main(int argc, char* argv[])
{
    try
    {
        CIntercomService service;
        CCustomCmd ddsCustomCmd(service);

        service.subscribeOnError([](const EErrorCode errorCode, const string& errorMsg)
        {
            cout << "DDS error received: error code: " << errorCode << ", error message: " << errorMsg << endl;
        });

        // subscribe to receive messages from DDS
        ddsCustomCmd.subscribe([](const string& msg, const string& /*condition*/, uint64_t /*senderId*/)
        {
            cout << "Received: \"" << msg << "\"" << endl;
        });

        service.start();

        char c;

        // setup reading from cin (enable raw mode)
        struct termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag &= ~ICANON; // disable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

        if (argc != 2)
        {
            PrintControlsHelp();
        }
        else
        {
            cin.putback(argv[1][0]);
        }

        while (cin >> c)
        {
            switch (c)
            {
                case 'c':
                    cout << " > checking state of the devices" << endl;
                    ddsCustomCmd.send("check-state", "");
                    break;
                case 'i':
                    cout << " > init devices" << endl;
                    ddsCustomCmd.send("INIT DEVICE", "");
                    break;
                case 'j':
                    cout << " > init tasks" << endl;
                    ddsCustomCmd.send("INIT TASK", "");
                    break;
                case 'p':
                    cout << " > pause devices" << endl;
                    ddsCustomCmd.send("PAUSE", "");
                    break;
                case 'r':
                    cout << " > run tasks" << endl;
                    ddsCustomCmd.send("RUN", "");
                    break;
                case 's':
                    cout << " > stop devices" << endl;
                    ddsCustomCmd.send("STOP", "");
                    break;
                case 't':
                    cout << " > reset tasks" << endl;
                    ddsCustomCmd.send("RESET TASK", "");
                    break;
                case 'd':
                    cout << " > reset devices" << endl;
                    ddsCustomCmd.send("RESET DEVICE", "");
                    break;
                case 'h':
                    cout << " > help" << endl;
                    PrintControlsHelp();
                    break;
                case 'q':
                    cout << " > end" << endl;
                    ddsCustomCmd.send("END", "");
                    break;
                default:
                    cout << "Invalid input: [" << c << "]" << endl;
                    PrintControlsHelp();
                    break;
            }

            if (argc == 2)
            {
                usleep(50000);
                return EXIT_SUCCESS;
            }
        }

        // disable raw mode
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag |= ICANON; // re-enable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    }
    catch (exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
