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
    cout << "[c] check states, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device, [b] bind, [x] connect" << endl;
    cout << "To quit press Ctrl+C" << endl;
}

int main(int argc, char* argv[])
{
    try {
        string sessionID;
        char command = ' ';
        string topologyPath;

        bpo::options_description options("fairmq-dds-command-ui options");
        options.add_options()
            ("session,s", bpo::value<string> (&sessionID)->required(), "DDS Session ID")
            ("command,c", bpo::value<char>   (&command)->default_value(' '), "Command character")
            ("path,p",    bpo::value<string> (&topologyPath)->default_value(""), "DDS Topology path to send command to")
            ("help,h", "Produce help message");

        bpo::variables_map vm;
        bpo::store(bpo::command_line_parser(argc, argv).options(options).run(), vm);

        if (vm.count("help")) {
            cout << "FairMQ DDS Command UI" << endl << options << endl;
            cout << "Commands: [c] check state, [o] dump config, [h] help, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device" << endl;
            return EXIT_SUCCESS;
        }

        bpo::notify(vm);

        CIntercomService service;
        CCustomCmd ddsCustomCmd(service);

        service.subscribeOnError([](const EErrorCode errorCode, const string& errorMsg) {
            cout << "DDS error received: error code: " << errorCode << ", error message: " << errorMsg << endl;
        });

        // subscribe to receive messages from DDS
        ddsCustomCmd.subscribe([](const string& msg, const string& /*condition*/, uint64_t /*senderId*/) {
            cout << "Received: " << endl << msg << endl;
        });

        service.start(sessionID);

        char c;

        // setup reading from cin (enable raw mode)
        struct termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag &= ~ICANON; // disable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

        if (command != ' ') {
            cin.putback(command);
        } else {
            PrintControlsHelp();
        }

        while (cin >> c) {
            switch (c) {
                case 'c':
                    cout << " > checking state of the devices" << endl;
                    ddsCustomCmd.send("check-state", topologyPath);
                    break;
                case 'o':
                    cout << " > dumping config of the devices" << endl;
                    ddsCustomCmd.send("dump-config", topologyPath);
                    break;
                case 'i':
                    cout << " > init devices" << endl;
                    ddsCustomCmd.send("INIT DEVICE", topologyPath);
                    break;
                case 'b':
                    cout << " > bind" << endl;
                    ddsCustomCmd.send("BIND", topologyPath);
                    break;
                case 'x':
                    cout << " > connect" << endl;
                    ddsCustomCmd.send("CONNECT", topologyPath);
                    break;
                case 'j':
                    cout << " > init tasks" << endl;
                    ddsCustomCmd.send("INIT TASK", topologyPath);
                    break;
                case 'r':
                    cout << " > run tasks" << endl;
                    ddsCustomCmd.send("RUN", topologyPath);
                    break;
                case 's':
                    cout << " > stop devices" << endl;
                    ddsCustomCmd.send("STOP", topologyPath);
                    break;
                case 't':
                    cout << " > reset tasks" << endl;
                    ddsCustomCmd.send("RESET TASK", topologyPath);
                    break;
                case 'd':
                    cout << " > reset devices" << endl;
                    ddsCustomCmd.send("RESET DEVICE", topologyPath);
                    break;
                case 'h':
                    cout << " > help" << endl;
                    PrintControlsHelp();
                    break;
                case 'q':
                    cout << " > end" << endl;
                    ddsCustomCmd.send("END", topologyPath);
                    break;
                default:
                    cout << "Invalid input: [" << c << "]" << endl;
                    PrintControlsHelp();
                    break;
            }

            if (command != ' ') {
                this_thread::sleep_for(chrono::milliseconds(100)); // give dds a chance to complete request
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
