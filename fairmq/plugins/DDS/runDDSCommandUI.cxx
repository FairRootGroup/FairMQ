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

#include <boost/program_options.hpp>

using namespace std;
using namespace dds::intercom_api;
namespace bpo = boost::program_options;

void PrintControlsHelp()
{
    cout << "Use keys to control the devices:" << endl;
    cout << "[c] check states, [o] dump config, [h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device" << endl;
    cout << "To quit press Ctrl+C" << endl;
}

int main(int argc, char* argv[])
{
    try {
        string sessionID;
        char commandChar;

        bpo::options_description options("fairmq-dds-command-ui options");
        options.add_options()
            ("session,s", bpo::value<string>(&sessionID)->required(), "DDS Session ID")
            ("command,c", bpo::value<char>  (&commandChar)->default_value(' '), "Command character")
            ("help,h", "Produce help message");

        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(options).run(), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            cout << "FairMQ DDS Command UI" << endl << options << endl;
            cout << "possible command characters: [c] check states, [o] dump config, [h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device" << endl;
            return EXIT_SUCCESS;
        }

        CIntercomService service;
        CCustomCmd ddsCustomCmd(service);

        service.subscribeOnError([](const EErrorCode errorCode, const string& errorMsg) {
            cout << "DDS error received: error code: " << errorCode << ", error message: " << errorMsg << endl;
        });

        // subscribe to receive messages from DDS
        ddsCustomCmd.subscribe([](const string& msg, const string& /*condition*/, uint64_t /*senderId*/) {
            cout << "Received: " << msg << endl;
        });

        service.start(sessionID);

        char c;

        // setup reading from cin (enable raw mode)
        struct termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag &= ~ICANON; // disable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

        if (commandChar != ' ') {
            cin.putback(commandChar);
        } else {
            PrintControlsHelp();
        }

        while (cin >> c) {
            switch (c) {
                case 'c':
                    cout << " > checking state of the devices" << endl;
                    ddsCustomCmd.send("check-state", "");
                    break;
                case 'o':
                    cout << " > dumping config of the devices" << endl;
                    ddsCustomCmd.send("dump-config", "");
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

            if (commandChar != ' ') {
                usleep(50000);
                return EXIT_SUCCESS;
            }
        }

        // disable raw mode
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag |= ICANON; // re-enable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    } catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
